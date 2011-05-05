/*
 * Copyright (C) 2010,2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Chris Lalancette <clalance@redhat.com>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <curl/curl.h>
#include <pthread.h>
#include "libdeltacloud.h"
#include "common.h"
#include "curl_action.h"

/***************** ERROR HANDLING ROUTINES ***************************/
/* On first initialization of the library we setup a per-thread local
 * variable to hold errors.  If one of the API calls subsequently fails,
 * then we set the per-thread variable with details of the failure.
 */
static int tlsinitialized = 0;

struct deltacloud_error *deltacloud_get_last_error(void)
{
  return pthread_getspecific(deltacloud_last_error);
}

const char *deltacloud_get_last_error_string(void)
{
  struct deltacloud_error *last;

  last = deltacloud_get_last_error();
  if (last != NULL)
    return last->details;
  return NULL;
}

static void xml_error(const char *name, const char *type, const char *details)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "%s for %s: %s", type, name, details) < 0) {
    tmp = "Failed parsing XML";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_XML_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void link_error(const char *name)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Failed to find the link for '%s'", name) < 0) {
    tmp = "Failed to find the link";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_URL_DOES_NOT_EXIST_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void data_error(const char *name)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Expected %s data, received nothing", name) < 0) {
    tmp = "Expected data, received nothing";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_GET_URL_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void find_by_name_error(const char *name, const char *type)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Failed to find '%s' in '%s' list", name, type) < 0) {
    tmp = "Failed to find the link";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_NAME_NOT_FOUND_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void oom_error(void)
{
  set_error(DELTACLOUD_OOM_ERROR, "Failed to allocate memory");
}

static void invalid_argument_error(const char *details)
{
  set_error(DELTACLOUD_INVALID_ARGUMENT_ERROR, details);
}

/********************** XML PARSING *********************************/
static char *getXPathString(const char *xpath, xmlXPathContextPtr ctxt)
{
  xmlXPathObjectPtr obj;
  xmlNodePtr relnode;
  char *ret;

  if ((ctxt == NULL) || (xpath == NULL))
    return NULL;

  relnode = ctxt->node;
  obj = xmlXPathEval(BAD_CAST xpath, ctxt);
  ctxt->node = relnode;
  if ((obj == NULL) || (obj->type != XPATH_STRING) ||
      (obj->stringval == NULL) || (obj->stringval[0] == 0)) {
    xmlXPathFreeObject(obj);
    return NULL;
  }
  ret = strdup((char *) obj->stringval);
  xmlXPathFreeObject(obj);

  return ret;
}

static int parse_xml(const char *xml_string, const char *name, void **data,
		     int (*cb)(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			       void **data), int multiple)
{
  xmlDocPtr xml;
  xmlNodePtr root;
  xmlXPathContextPtr ctxt = NULL;
  int ret = -1;
  int rc;
  xmlErrorPtr last;
  char *msg;

  xml = xmlReadDoc(BAD_CAST xml_string, name, NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    last = xmlGetLastError();
    if (last != NULL)
      msg = last->message;
    else
      msg = "unknown error";
    xml_error(name, "Failed to parse XML", msg);
    return -1;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    last = xmlGetLastError();
    if (last != NULL)
      msg = last->message;
    else
      msg = "unknown error";
    xml_error(name, "Failed to get the root element", msg);
    goto cleanup;
  }

  if (STRNEQ((const char *)root->name, name)) {
    xml_error(name, "Failed to get expected root element", (char *)root->name);
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    last = xmlGetLastError();
    if (last != NULL)
      msg = last->message;
    else
      msg = "unknown error";
    xml_error(name, "Failed to initialize XPath context", msg);
    goto cleanup;
  }

  /* if "multiple" is true, then the XML looks something like:
   * <instances> <instance> ... </instance> </instances>"
   * if "multiple" is false, then the XML looks something like:
   * <instance> ... </instance>
   */
  if (multiple)
    rc = cb(root->children, ctxt, data);
  else
    rc = cb(root, ctxt, data);

  if (rc < 0)
    /* the callbacks are expected to have set their own error */
    goto cleanup;

  ret = 0;

 cleanup:
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);

  return ret;
}

static int parse_error_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data)
{
  char **msg = (char **)data;

  *msg = getXPathString("string(/error/message)", ctxt);

  if (*msg == NULL)
    *msg = strdup("Unknown error");

  return 0;
}

static int is_error_xml(const char *xml)
{
  return STRPREFIX(xml, "<error");
}

static void set_xml_error(const char *xml, int type)
{
  char *errmsg = NULL;

  if (parse_xml(xml, "error", (void **)&errmsg, parse_error_xml, 1) < 0)
    errmsg = strdup("Unknown error");

  set_error(type, errmsg);

  SAFE_FREE(errmsg);
}

static int parse_feature_xml(xmlNodePtr featurenode,
			     struct deltacloud_feature **features)
{
  struct deltacloud_feature thisfeature;
  int listret;

  memset(&thisfeature, 0, sizeof(struct deltacloud_feature));

  while (featurenode != NULL) {
    if (featurenode->type == XML_ELEMENT_NODE &&
	STREQ((const char *)featurenode->name, "feature")) {
      thisfeature.name = (char *)xmlGetProp(featurenode, BAD_CAST "name");
      listret = add_to_feature_list(features, &thisfeature);
      free_feature(&thisfeature);
      if (listret < 0) {
	free_feature_list(features);
	oom_error();
	return -1;
      }
    }
    featurenode = featurenode->next;
  }

  return 0;
}

static int parse_api_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data)
{
  struct deltacloud_api **api = (struct deltacloud_api **)data;
  struct deltacloud_link thislink;
  xmlNodePtr linknode;
  int ret = -1;
  int listret;

  memset(&thislink, 0, sizeof(struct deltacloud_link));

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "api")) {
      (*api)->driver = (char *)xmlGetProp(cur, BAD_CAST "driver");
      (*api)->version = (char *)xmlGetProp(cur, BAD_CAST "version");

      linknode = cur->children;
      while (linknode != NULL) {
	if (linknode->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)linknode->name, "link")) {

	  thislink.href = (char *)xmlGetProp(linknode, BAD_CAST "href");
	  thislink.rel = (char *)xmlGetProp(linknode, BAD_CAST "rel");
	  if (parse_feature_xml(linknode->children, &(thislink.features)) < 0) {
	    /* parse_feature_xml already set the error */
	    free_link(&thislink);
	    goto cleanup;
	  }

	  listret = add_to_link_list(&((*api)->links), &thislink);
	  free_link(&thislink);
	  if (listret < 0) {
	    oom_error();
	    goto cleanup;
	  }
	}
	linknode = linknode->next;
      }
    }

    cur = cur->next;
  }

  ret = 0;

 cleanup:
  /* if we hit some kind of failure here, in theory we would want to clean
   * up the api structure.  However, deltacloud_initialize() also does some
   * allocation of memory, so it does a deltacloud_free() on error, so we leave
   * it up to deltacloud_initialize() to cleanup.
   */

  return ret;
}

#define valid_arg(x) ((x == NULL) ? invalid_argument_error(#x " cannot be NULL"), 0 : 1)

static int internal_destroy(const char *href, const char *user,
			    const char *password)
{
  char *data = NULL;
  int ret = -1;

  /* in deltacloud the destroy action is a DELETE method, so we need
   * to use a different implementation
   */
  data = delete_url(href, user, password);
  if (data == NULL)
    /* delete_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_DELETE_URL_ERROR);
    goto cleanup;
  }
  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

/*
 * An internal function for fetching all of the elements of a particular
 * type.  Note that although relname and rootname is the same for almost
 * all types, there are a couple of them that don't conform to this pattern.
 */
static int internal_get(struct deltacloud_api *api, const char *relname,
			const char *rootname,
			int (*xml_cb)(xmlNodePtr, xmlXPathContextPtr, void **),
			void **output)
{
  struct deltacloud_link *thislink = NULL;
  char *data = NULL;
  int ret = -1;

  /* we only check api and output here, as those are the only parameters from
   * the user
   */
  if (!valid_arg(api) || !valid_arg(output))
    return -1;

  deltacloud_for_each(thislink, api->links) {
    if (STREQ(thislink->rel, relname))
      break;
  }
  if (thislink == NULL) {
    link_error(relname);
    return -1;
  }

  if (get_url(thislink->href, api->user, api->password, &data) != 0)
    /* get_url sets its own errors, so don't overwrite it here */
    return -1;

  if (data == NULL) {
    /* if we made it here, it means that the transfer was successful (ret
     * was 0), but the data that we expected wasn't returned.  This is probably
     * a deltacloud server bug, so just set an error and bail out
     */
    data_error(relname);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  *output = NULL;
  if (parse_xml(data, rootname, output, xml_cb, 1) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

static int internal_get_by_id(struct deltacloud_api *api, const char *id,
			      const char *name,
			      int (*parse_cb)(const char *, void *),
			      void *output)
{
  char *url = NULL;
  char *data = NULL;
  char *safeid;
  int ret = -1;

  /* we only check api, id, and output here, as those are the only parameters
   * from the user
   */
  if (!valid_arg(api) || !valid_arg(id) || !valid_arg(output))
    return -1;

  safeid = curl_escape(id, 0);
  if (safeid == NULL) {
    oom_error();
    return -1;
  }

  if (asprintf(&url, "%s/%s/%s", api->url, name, safeid) < 0) {
    oom_error();
    goto cleanup;
  }

  if (get_url(url, api->user, api->password, &data) != 0)
    /* get_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (data == NULL) {
    /* if we made it here, it means that the transfer was successful (ret
     * was 0), but the data that we expected wasn't returned.  This is probably
     * a deltacloud server bug, so just set an error and bail out
     */
    data_error(name);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_cb(data, output) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  SAFE_FREE(url);
  curl_free(safeid);

  return ret;
}

int deltacloud_initialize(struct deltacloud_api *api, char *url, char *user,
			  char *password)
{
  char *data = NULL;
  int ret = -1;

  if (!tlsinitialized) {
    tlsinitialized = 1;
    if (pthread_key_create(&deltacloud_last_error,
			   deltacloud_error_free_data) != 0) {
      dcloudprintf("Failed to initialize error handler\n");
      return -1;
    }
  }

  if (!valid_arg(api) || !valid_arg(url) || !valid_arg(user) ||
      !valid_arg(password))
    return -1;

  memset(api, 0, sizeof(struct deltacloud_api));

  api->url = strdup(url);
  if (api->url == NULL) {
    oom_error();
    return -1;
  }
  api->user = strdup(user);
  if (api->user == NULL) {
    oom_error();
    goto cleanup;
  }
  api->password = strdup(password);
  if (api->password == NULL) {
    oom_error();
    goto cleanup;
  }
  api->links = NULL;

  if (get_url(api->url, api->user, api->password, &data) != 0)
    /* get_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (data == NULL) {
    /* if we made it here, it means that the transfer was successful (ret
     * was 0), but the data that we expected wasn't returned.  This is probably
     * a deltacloud server bug, so just set an error and bail out
     */
    set_error(DELTACLOUD_GET_URL_ERROR, "Expected link data, received nothing");
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml(data, "api", (void **)&api, parse_api_xml, 0) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  if (ret < 0)
    deltacloud_free(api);
  return ret;
}

static int parse_addresses_xml(xmlNodePtr instance, xmlXPathContextPtr ctxt,
			       struct deltacloud_address **addresses)
{
  struct deltacloud_address thisaddr;
  xmlNodePtr oldnode, cur;
  int listret;
  int ret = -1;

  /* you might convince yourself that memory corruption is possible after the
   * first iteration of the loop.  That is, after that first iteration, the
   * structure could point to uninitialized memory since we don't re-zero it.
   * Luckily the "free" function zeros out the memory, so we are good to go.
   */
  memset(&thisaddr, 0, sizeof(struct deltacloud_address));

  *addresses = NULL;

  oldnode = ctxt->node;

  ctxt->node = instance;
  cur = instance->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "address")) {
      thisaddr.address = getXPathString("string(./address)", ctxt);
      if (thisaddr.address != NULL) {
	listret = add_to_address_list(addresses, &thisaddr);
	free_address(&thisaddr);
	if (listret < 0) {
	  oom_error();
	  goto cleanup;
	}
      }
      /* address is allowed to be NULL, so skip it here */
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    free_address_list(addresses);

  return ret;
}

static int parse_actions_xml(xmlNodePtr instance,
			     struct deltacloud_action **actions)
{
  struct deltacloud_action thisaction;
  xmlNodePtr cur;
  int ret = -1;
  int listret;

  /* you might convince yourself that memory corruption is possible after the
   * first iteration of the loop.  That is, after that first iteration, the
   * structure could point to uninitialized memory since we don't re-zero it.
   * Luckily the "free" function zeros out the memory, so we are good to go.
   */
  memset(&thisaction, 0, sizeof(struct deltacloud_action));

  cur = instance->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "link")) {

      thisaction.href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thisaction.rel = (char *)xmlGetProp(cur, BAD_CAST "rel");
      thisaction.method = (char *)xmlGetProp(cur, BAD_CAST "method");

      listret = add_to_action_list(actions, &thisaction);
      free_action(&thisaction);

      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_action_list(actions);

  return ret;
}

static int parse_hardware_profile_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data);
static int parse_instance_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			      void **data)
{
  struct deltacloud_instance **instances = (struct deltacloud_instance **)data;
  struct deltacloud_instance thisinst;
  struct deltacloud_hardware_profile *hwp = NULL;
  xmlNodePtr oldnode;
  xmlXPathObjectPtr hwpset, actionset, pubset, privset;
  int listret;
  int ret = -1;

  /* you might convince yourself that memory corruption is possible after the
   * first iteration of the loop.  That is, after that first iteration, the
   * structure could point to uninitialized memory since we don't re-zero it.
   * Luckily the "free" function zeros out the memory, so we are good to go.
   */
  memset(&thisinst, 0, sizeof(struct deltacloud_instance));

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "instance")) {

      ctxt->node = cur;

      thisinst.href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thisinst.id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thisinst.name = getXPathString("string(./name)", ctxt);
      thisinst.owner_id = getXPathString("string(./owner_id)", ctxt);
      thisinst.image_id = getXPathString("string(./image/@id)", ctxt);
      thisinst.image_href = getXPathString("string(./image/@href)", ctxt);
      thisinst.realm_id = getXPathString("string(./realm/@id)", ctxt);
      thisinst.realm_href = getXPathString("string(./realm/@href)", ctxt);
      thisinst.state = getXPathString("string(./state)", ctxt);

      hwpset = xmlXPathEval(BAD_CAST "./hardware_profile", ctxt);
      if (hwpset && hwpset->type == XPATH_NODESET && hwpset->nodesetval &&
	  hwpset->nodesetval->nodeNr == 1) {
	parse_hardware_profile_xml(hwpset->nodesetval->nodeTab[0], ctxt,
				   (void **)&hwp);
	copy_hardware_profile(&(thisinst.hwp), hwp);
	deltacloud_free_hardware_profile_list(&hwp);
      }
      xmlXPathFreeObject(hwpset);

      actionset = xmlXPathEval(BAD_CAST "./actions", ctxt);
      if (actionset && actionset->type == XPATH_NODESET &&
	  actionset->nodesetval && actionset->nodesetval->nodeNr == 1)
	parse_actions_xml(actionset->nodesetval->nodeTab[0],
			  &(thisinst.actions));
      xmlXPathFreeObject(actionset);

      pubset = xmlXPathEval(BAD_CAST "./public_addresses", ctxt);
      if (pubset && pubset->type == XPATH_NODESET && pubset->nodesetval &&
	  pubset->nodesetval->nodeNr == 1)
	parse_addresses_xml(pubset->nodesetval->nodeTab[0], ctxt,
			    &(thisinst.public_addresses));
      xmlXPathFreeObject(pubset);

      privset = xmlXPathEval(BAD_CAST "./private_addresses", ctxt);
      if (privset && privset->type == XPATH_NODESET && privset->nodesetval &&
	  privset->nodesetval->nodeNr == 1)
	parse_addresses_xml(privset->nodesetval->nodeTab[0], ctxt,
			    &(thisinst.private_addresses));
      xmlXPathFreeObject(privset);

      listret = add_to_instance_list(instances, &thisinst);
      deltacloud_free_instance(&thisinst);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_instance_list(instances);
  ctxt->node = oldnode;

  return ret;
}

int deltacloud_get_instances(struct deltacloud_api *api,
			     struct deltacloud_instance **instances)
{
  return internal_get(api, "instances", "instances", parse_instance_xml,
		      (void **)instances);
}

static int parse_one_instance(const char *data, void *output)
{
  int ret = -1;
  struct deltacloud_instance *newinstance = (struct deltacloud_instance *)output;
  struct deltacloud_instance *tmpinstance = NULL;

  if (parse_xml(data, "instance", (void **)&tmpinstance,
		parse_instance_xml, 0) < 0)
    goto cleanup;

  if (copy_instance(newinstance, tmpinstance) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_instance_list(&tmpinstance);

  return ret;
}

int deltacloud_get_instance_by_id(struct deltacloud_api *api, const char *id,
				  struct deltacloud_instance *instance)
{
  return internal_get_by_id(api, id, "instances", parse_one_instance, instance);
}

static int parse_realm_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			   void **data)
{
  struct deltacloud_realm **realms = (struct deltacloud_realm **)data;
  struct deltacloud_realm thisrealm;
  xmlNodePtr oldnode;
  int ret = -1;
  int listret;

  /* you might convince yourself that memory corruption is possible after the
   * first iteration of the loop.  That is, after that first iteration, the
   * structure could point to uninitialized memory since we don't re-zero it.
   * Luckily the "free" function zeros out the memory, so we are good to go.
   */
  memset(&thisrealm, 0, sizeof(struct deltacloud_realm));

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "realm")) {

      ctxt->node = cur;

      thisrealm.href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thisrealm.id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thisrealm.name = getXPathString("string(./name)", ctxt);
      thisrealm.state = getXPathString("string(./state)", ctxt);
      thisrealm.limit = getXPathString("string(./limit)", ctxt);

      listret = add_to_realm_list(realms, &thisrealm);
      deltacloud_free_realm(&thisrealm);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_realm_list(realms);
  ctxt->node = oldnode;

  return ret;
}

int deltacloud_get_realms(struct deltacloud_api *api,
			  struct deltacloud_realm **realms)
{
  return internal_get(api, "realms", "realms", parse_realm_xml,
		      (void **)realms);
}

static int parse_one_realm(const char *data, void *output)
{
  int ret = -1;
  struct deltacloud_realm *newrealm = (struct deltacloud_realm *)output;
  struct deltacloud_realm *tmprealm = NULL;

  if (parse_xml(data, "realm", (void **)&tmprealm, parse_realm_xml, 0) < 0)
    goto cleanup;

  if (copy_realm(newrealm, tmprealm) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_realm_list(&tmprealm);

  return ret;
}

int deltacloud_get_realm_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_realm *realm)
{
  return internal_get_by_id(api, id, "realms", parse_one_realm, realm);
}

static int parse_hwp_params_enums_ranges(xmlNodePtr property,
					 struct deltacloud_property *prop)
{
  struct deltacloud_property_param thisparam;
  struct deltacloud_property_enum thisenum;
  struct deltacloud_property_range thisrange;
  xmlNodePtr enum_cur;
  int listret;
  int ret = -1;

  memset(&thisparam, 0, sizeof(struct deltacloud_property_param));
  memset(&thisenum, 0, sizeof(struct deltacloud_property_enum));
  memset(&thisrange, 0, sizeof(struct deltacloud_property_range));

  while (property != NULL) {
    if (property->type == XML_ELEMENT_NODE) {
      if (STREQ((const char *)property->name, "param")) {
	thisparam.href = (char *)xmlGetProp(property, BAD_CAST "href");
	thisparam.method = (char *)xmlGetProp(property, BAD_CAST "method");
	thisparam.name = (char *)xmlGetProp(property, BAD_CAST "name");
	thisparam.operation = (char *)xmlGetProp(property,
						 BAD_CAST "operation");

	listret = add_to_param_list(&prop->params, &thisparam);
	free_param(&thisparam);
	if (listret < 0) {
	  oom_error();
	  goto cleanup;
	}
      }
      else if(STREQ((const char *)property->name, "enum")) {
	enum_cur = property->children;
	while (enum_cur != NULL) {
	  if (enum_cur->type == XML_ELEMENT_NODE &&
	      STREQ((const char *)enum_cur->name, "entry")) {
	    thisenum.value = (char *)xmlGetProp(enum_cur, BAD_CAST "value");

	    listret = add_to_enum_list(&prop->enums, &thisenum);
	    free_enum(&thisenum);
	    if (listret < 0) {
	      oom_error();
	      goto cleanup;
	    }
	  }

	  enum_cur = enum_cur->next;
	}
      }
      else if (STREQ((const char *)property->name, "range")) {
	thisrange.first = (char *)xmlGetProp(property, BAD_CAST "first");
	thisrange.last = (char *)xmlGetProp(property, BAD_CAST "last");

	listret = add_to_range_list(&prop->ranges, &thisrange);
	free_range(&thisrange);
	if (listret < 0) {
	  oom_error();
	  goto cleanup;
	}
      }
    }
    property = property->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0) {
    free_param_list(&prop->params);
    free_enum_list(&prop->enums);
    free_range_list(&prop->ranges);
  }

  return ret;
}

static int parse_hardware_profile_properties(xmlNodePtr hwp,
					     struct deltacloud_property **props)
{
  xmlNodePtr profile_cur;
  struct deltacloud_property thisprop;
  int listret;
  int ret = -1;

  memset(&thisprop, 0, sizeof(struct deltacloud_property));

  profile_cur = hwp->children;

  while (profile_cur != NULL) {
    if (profile_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)profile_cur->name, "property")) {
      thisprop.kind = (char *)xmlGetProp(profile_cur, BAD_CAST "kind");
      thisprop.name = (char *)xmlGetProp(profile_cur, BAD_CAST "name");
      thisprop.unit = (char *)xmlGetProp(profile_cur, BAD_CAST "unit");
      thisprop.value = (char *)xmlGetProp(profile_cur, BAD_CAST "value");

      if (parse_hwp_params_enums_ranges(profile_cur->children, &thisprop) < 0) {
	/* parse_hwp_params_enums_ranges already set the error */
	free_prop(&thisprop);
	goto cleanup;
      }

      listret = add_to_property_list(props, &thisprop);
      free_prop(&thisprop);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }

    profile_cur = profile_cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_property_list(props);
  return ret;
}

static int parse_hardware_profile_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data)
{
  struct deltacloud_hardware_profile **profiles = (struct deltacloud_hardware_profile **)data;
  struct deltacloud_hardware_profile thishwp;
  xmlNodePtr oldnode;
  int ret = -1;
  int listret;

  memset(&thishwp, 0, sizeof(struct deltacloud_hardware_profile));

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "hardware_profile")) {

      ctxt->node = cur;

      thishwp.href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thishwp.id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thishwp.name = getXPathString("string(./name)", ctxt);

      if (parse_hardware_profile_properties(cur, &(thishwp.properties)) < 0) {
	/* parse_hardware_profile_properties already set the error */
	deltacloud_free_hardware_profile(&thishwp);
	goto cleanup;
      }

      listret = add_to_hardware_profile_list(profiles, &thishwp);
      deltacloud_free_hardware_profile(&thishwp);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }

    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_hardware_profile_list(profiles);

  return ret;
}

int deltacloud_get_hardware_profiles(struct deltacloud_api *api,
				     struct deltacloud_hardware_profile **profiles)
{
  return internal_get(api, "hardware_profiles", "hardware_profiles",
		      parse_hardware_profile_xml, (void **)profiles);
}

static int parse_one_hwp(const char *data, void *output)
{
  int ret = -1;
  struct deltacloud_hardware_profile *newprofile = (struct deltacloud_hardware_profile *)output;
  struct deltacloud_hardware_profile *tmpprofile = NULL;

  if (parse_xml(data, "hardware_profile", (void **)&tmpprofile,
		parse_hardware_profile_xml, 0) < 0)
    goto cleanup;

  if (copy_hardware_profile(newprofile, tmpprofile) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_hardware_profile_list(&tmpprofile);

  return ret;
}

int deltacloud_get_hardware_profile_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_hardware_profile *profile)
{
  return internal_get_by_id(api, id, "hardware_profiles", parse_one_hwp,
			    profile);
}

static int parse_image_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			   void **data)
{
  struct deltacloud_image **images = (struct deltacloud_image **)data;
  struct deltacloud_image thisimage;
  xmlNodePtr oldnode;
  int listret;
  int ret = -1;

  /* you might convince yourself that memory corruption is possible after the
   * first iteration of the loop.  That is, after that first iteration, the
   * structure could point to uninitialized memory since we don't re-zero it.
   * Luckily the "free" function zeros out the memory, so we are good to go.
   */
  memset(&thisimage, 0, sizeof(struct deltacloud_image));

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "image")) {

      ctxt->node = cur;

      thisimage.href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thisimage.id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thisimage.description = getXPathString("string(./description)", ctxt);
      thisimage.architecture = getXPathString("string(./architecture)", ctxt);
      thisimage.owner_id = getXPathString("string(./owner_id)", ctxt);
      thisimage.name = getXPathString("string(./name)", ctxt);
      thisimage.state = getXPathString("string(./state)", ctxt);

      listret = add_to_image_list(images, &thisimage);
      deltacloud_free_image(&thisimage);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_image_list(images);

  return ret;
}

int deltacloud_get_images(struct deltacloud_api *api,
			  struct deltacloud_image **images)
{
  return internal_get(api, "images", "images", parse_image_xml,
		      (void **)images);
}

static int parse_one_image(const char *data, void *output)
{
  int ret = -1;
  struct deltacloud_image *newimage = (struct deltacloud_image *)output;
  struct deltacloud_image *tmpimage = NULL;

  if (parse_xml(data, "image", (void **)&tmpimage, parse_image_xml, 0) < 0)
    goto cleanup;

  if (copy_image(newimage, tmpimage) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_image_list(&tmpimage);

  return ret;
}

int deltacloud_get_image_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_image *image)
{
  return internal_get_by_id(api, id, "images", parse_one_image, image);
}

static int parse_instance_state_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void **data)
{
  struct deltacloud_instance_state **instance_states = (struct deltacloud_instance_state **)data;
  struct deltacloud_instance_state thisstate;
  struct deltacloud_instance_state_transition thistrans;
  xmlNodePtr state_cur;
  int ret = -1;
  int listret;

  /* you might convince yourself that memory corruption is possible after the
   * first iteration of the loop.  That is, after that first iteration, the
   * structure could point to uninitialized memory since we don't re-zero it.
   * Luckily the "free" function zeros out the memory, so we are good to go.
   */
  memset(&thisstate, 0, sizeof(struct deltacloud_instance_state));
  memset(&thistrans, 0, sizeof(struct deltacloud_instance_state_transition));

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "state")) {

      thisstate.name = (char *)xmlGetProp(cur, BAD_CAST "name");

      state_cur = cur->children;
      while (state_cur != NULL) {
	if (state_cur->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)state_cur->name, "transition")) {
	  thistrans.action = (char *)xmlGetProp(state_cur, BAD_CAST "action");
	  thistrans.to = (char *)xmlGetProp(state_cur, BAD_CAST "to");
	  thistrans.auto_bool = (char *)xmlGetProp(state_cur, BAD_CAST "auto");
	  listret = add_to_transition_list(&(thisstate.transitions),
					   &thistrans);
	  free_transition(&thistrans);
	  if (listret < 0) {
	    deltacloud_free_instance_state(&thisstate);
	    oom_error();
	    goto cleanup;
	  }
	}
	state_cur = state_cur->next;
      }

      listret = add_to_instance_state_list(instance_states, &thisstate);
      deltacloud_free_instance_state(&thisstate);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_instance_state_list(instance_states);

  return ret;
}

int deltacloud_get_instance_states(struct deltacloud_api *api,
				   struct deltacloud_instance_state **instance_states)
{
  return internal_get(api, "instance_states", "states",
		      parse_instance_state_xml, (void **)instance_states);
}

int deltacloud_get_instance_state_by_name(struct deltacloud_api *api,
					  const char *name,
					  struct deltacloud_instance_state *instance_state)
{
  struct deltacloud_instance_state *statelist = NULL;
  struct deltacloud_instance_state *state;
  int instance_ret;
  int ret = -1;

  /* despite the fact that 'name' is an input from the user, we don't
   * need to escape it since we are never using it as a URL
   */

  if (!valid_arg(api) || !valid_arg(name) || !valid_arg(instance_state))
    return -1;

  instance_ret = deltacloud_get_instance_states(api, &statelist);
  if (instance_ret < 0)
    return instance_ret;

  deltacloud_for_each(state, statelist) {
    if (STREQ(state->name, name))
      break;
  }
  if (state == NULL) {
    find_by_name_error(name, "instance_state");
    goto cleanup;
  }

  if (copy_instance_state(instance_state, state) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_instance_state_list(&statelist);
  return ret;
}

static int parse_storage_volume_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void **data)
{
  struct deltacloud_storage_volume **storage_volumes = (struct deltacloud_storage_volume **)data;
  struct deltacloud_storage_volume thisvolume;
  xmlNodePtr oldnode;
  int ret = -1;
  int listret;

  memset(&thisvolume, 0, sizeof(struct deltacloud_storage_volume));

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "storage_volume")) {

      ctxt->node = cur;

      thisvolume.href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thisvolume.id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thisvolume.created = getXPathString("string(./created)", ctxt);
      thisvolume.state = getXPathString("string(./state)", ctxt);
      thisvolume.capacity.unit = getXPathString("string(./capacity/@unit)",
						ctxt);
      thisvolume.capacity.size = getXPathString("string(./capacity)", ctxt);
      thisvolume.device = getXPathString("string(./device)", ctxt);
      thisvolume.instance_href = getXPathString("string(./instance/@href)",
						ctxt);
      thisvolume.realm_id = getXPathString("string(./realm_id)", ctxt);
      thisvolume.mount.instance_href = getXPathString("string(./mount/instance/@href)",
					   ctxt);
      thisvolume.mount.instance_id = getXPathString("string(./mount/instance/@id)",
						    ctxt);
      thisvolume.mount.device_name = getXPathString("string(./mount/device/@name)",
						    ctxt);

      listret = add_to_storage_volume_list(storage_volumes, &thisvolume);
      deltacloud_free_storage_volume(&thisvolume);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_storage_volume_list(storage_volumes);

  return ret;
}

int deltacloud_get_storage_volumes(struct deltacloud_api *api,
				   struct deltacloud_storage_volume **storage_volumes)
{
  return internal_get(api, "storage_volumes", "storage_volumes",
		      parse_storage_volume_xml, (void **)storage_volumes);
}

static int parse_one_storage_volume(const char *data, void *output)
{
  int ret = -1;
  struct deltacloud_storage_volume *newvolume = (struct deltacloud_storage_volume *)output;
  struct deltacloud_storage_volume *tmpvolume = NULL;

  if (parse_xml(data, "storage_volume", (void **)&tmpvolume,
		parse_storage_volume_xml, 0) < 0)
    goto cleanup;

  if (copy_storage_volume(newvolume, tmpvolume) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_storage_volume_list(&tmpvolume);

  return ret;
}

int deltacloud_get_storage_volume_by_id(struct deltacloud_api *api,
					const char *id,
					struct deltacloud_storage_volume *storage_volume)
{
  return internal_get_by_id(api, id, "storage_volumes",
			    parse_one_storage_volume, storage_volume);
}

static int parse_storage_snapshot_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data)
{
  struct deltacloud_storage_snapshot **storage_snapshots = (struct deltacloud_storage_snapshot **)data;
  struct deltacloud_storage_snapshot thissnapshot;
  int ret = -1;
  xmlNodePtr oldnode;
  int listret;

  memset(&thissnapshot, 0, sizeof(struct deltacloud_storage_snapshot));

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "storage_snapshot")) {

      ctxt->node = cur;

      thissnapshot.href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thissnapshot.id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thissnapshot.created = getXPathString("string(./created)", ctxt);
      thissnapshot.state = getXPathString("string(./state)", ctxt);
      thissnapshot.storage_volume_href = getXPathString("string(./storage_volume/@href)",
					   ctxt);
      thissnapshot.storage_volume_id = getXPathString("string(./storage_volume/@id)",
						      ctxt);

      listret = add_to_storage_snapshot_list(storage_snapshots, &thissnapshot);
      deltacloud_free_storage_snapshot(&thissnapshot);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_storage_snapshot_list(storage_snapshots);

  return ret;
}

int deltacloud_get_storage_snapshots(struct deltacloud_api *api,
				     struct deltacloud_storage_snapshot **storage_snapshots)
{
  return internal_get(api, "storage_snapshots", "storage_snapshots",
		      parse_storage_snapshot_xml, (void **)storage_snapshots);
}

static int parse_one_storage_snapshot(const char *data, void *output)
{
  int ret = -1;
  struct deltacloud_storage_snapshot *newsnapshot = (struct deltacloud_storage_snapshot *)output;
  struct deltacloud_storage_snapshot *tmpsnapshot = NULL;

  if (parse_xml(data, "storage_snapshot", (void **)&tmpsnapshot,
		parse_storage_snapshot_xml, 0) < 0)
    goto cleanup;

  if (copy_storage_snapshot(newsnapshot, tmpsnapshot) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_storage_snapshot_list(&tmpsnapshot);

  return ret;
}

int deltacloud_get_storage_snapshot_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_storage_snapshot *storage_snapshot)
{
  return internal_get_by_id(api, id, "storage_snapshots",
			    parse_one_storage_snapshot, storage_snapshot);
}

static int parse_key_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data)
{
  struct deltacloud_key **keys = (struct deltacloud_key **)data;
  struct deltacloud_key thiskey;
  int ret = -1;
  xmlNodePtr oldnode;
  int listret;

  /* you might convince yourself that memory corruption is possible after the
   * first iteration of the loop.  That is, after that first iteration, the
   * structure could point to uninitialized memory since we don't re-zero it.
   * Luckily the "free" function zeros out the memory, so we are good to go.
   */
  memset(&thiskey, 0, sizeof(struct deltacloud_key));

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "key")) {

      ctxt->node = cur;

      thiskey.href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thiskey.id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thiskey.type = (char *)xmlGetProp(cur, BAD_CAST "type");
      thiskey.state = getXPathString("string(./state)", ctxt);
      thiskey.fingerprint = getXPathString("string(./fingerprint)", ctxt);

      listret = add_to_key_list(keys, &thiskey);
      deltacloud_free_key(&thiskey);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_key_list(keys);

  return ret;
}

int deltacloud_get_keys(struct deltacloud_api *api,
			struct deltacloud_key **keys)
{
  return internal_get(api, "keys", "keys", parse_key_xml, (void **)keys);
}

static int parse_one_key(const char *data, void *output)
{
  int ret = -1;
  struct deltacloud_key *newkey = (struct deltacloud_key *)output;
  struct deltacloud_key *tmpkey = NULL;

  if (parse_xml(data, "key", (void **)&tmpkey, parse_key_xml, 0) < 0)
    goto cleanup;

  if (copy_key(newkey, tmpkey) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_key_list(&tmpkey);

  return ret;
}

int deltacloud_create_key(struct deltacloud_api *api, const char *name,
			  struct deltacloud_create_parameter *params,
			  int params_length)
{
  struct deltacloud_link *thislink;
  size_t param_string_length;
  FILE *paramfp;
  int ret = -1;
  char *data = NULL;
  char *param_string = NULL;
  char *safevalue = NULL;

  if (!valid_arg(api) || !valid_arg(name))
    return -1;

  deltacloud_for_each(thislink, api->links) {
    if (STREQ(thislink->rel, "keys"))
      break;
  }
  if (thislink == NULL) {
    link_error("keys");
    return -1;
  }

  paramfp = open_memstream(&param_string, &param_string_length);
  if (paramfp == NULL) {
    oom_error();
    return -1;
  }

  /* since the parameters come from the user, we must not trust them and
   * URL escape them before use
   */

  safevalue = curl_escape(name, 0);
  if (safevalue == NULL) {
      oom_error();
      goto cleanup;
  }
  fprintf(paramfp, "name=%s", safevalue);
  SAFE_FREE(safevalue);

  fclose(paramfp);
  paramfp = NULL;

  if (post_url(thislink->href, api->user, api->password, param_string,
	       &data, NULL) != 0)
    /* post_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (data != NULL && is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_POST_URL_ERROR);
    goto cleanup;
  }

  ret = 0;

 cleanup:
  if (paramfp != NULL)
    fclose(paramfp);
  SAFE_FREE(param_string);
  SAFE_FREE(data);

  return ret;
}

int deltacloud_key_destroy(struct deltacloud_api *api,
			   struct deltacloud_key *key)
{
  if (!valid_arg(api) || !valid_arg(key))
    return -1;

  return internal_destroy(key->href, api->user, api->password);
}

int deltacloud_get_key_by_id(struct deltacloud_api *api, const char *id,
			     struct deltacloud_key *key)
{
  return internal_get_by_id(api, id, "keys", parse_one_key, key);
}

static int parse_driver_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			    void **data)
{
  struct deltacloud_driver **drivers = (struct deltacloud_driver **)data;
  struct deltacloud_driver thisdriver;
  struct deltacloud_driver_provider thisprovider;
  int ret = -1;
  xmlNodePtr oldnode, providernode;
  int listret;

  oldnode = ctxt->node;

  /* you might convince yourself that memory corruption is possible after the
   * first iteration of the loop.  That is, after that first iteration, the
   * structure could point to uninitialized memory since we don't re-zero it.
   * Luckily the "free" function zeros out the memory, so we are good to go.
   */
  memset(&thisdriver, 0, sizeof(struct deltacloud_driver));
  memset(&thisprovider, 0, sizeof(struct deltacloud_driver_provider));

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "driver")) {

      ctxt->node = cur;

      thisdriver.href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thisdriver.id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thisdriver.name = getXPathString("string(./name)", ctxt);

      providernode = cur->children;
      while (providernode != NULL) {
	if (providernode->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)providernode->name, "provider")) {

	  thisprovider.id = (char *)xmlGetProp(providernode, BAD_CAST "id");

	  listret = add_to_provider_list(&(thisdriver.providers),
					 &thisprovider);
	  free_provider(&thisprovider);
	  if (listret < 0) {
	    free_provider_list(&(thisdriver.providers));
	    oom_error();
	    goto cleanup;
	  }
	}
	providernode = providernode->next;
      }

      listret = add_to_driver_list(drivers, &thisdriver);
      deltacloud_free_driver(&thisdriver);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_driver_list(drivers);

  return ret;
}

int deltacloud_get_drivers(struct deltacloud_api *api,
			   struct deltacloud_driver **drivers)
{
  return internal_get(api, "drivers", "drivers", parse_driver_xml,
		      (void **)drivers);
}

static int parse_one_driver(const char *data, void *output)
{
  int ret = -1;
  struct deltacloud_driver *newdriver = (struct deltacloud_driver *)output;
  struct deltacloud_driver *tmpdriver = NULL;

  if (parse_xml(data, "driver", (void **)&tmpdriver, parse_driver_xml, 0) < 0)
    goto cleanup;

  if (copy_driver(newdriver, tmpdriver) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_driver_list(&tmpdriver);

  return ret;
}

int deltacloud_get_driver_by_id(struct deltacloud_api *api, const char *id,
				struct deltacloud_driver *driver)
{
  return internal_get_by_id(api, id, "drivers", parse_one_driver, driver);
}

int deltacloud_prepare_parameter(struct deltacloud_create_parameter *param,
				 const char *name, const char *value)
{
  param->name = strdup(name);
  if (param->name == NULL) {
    oom_error();
    return -1;
  }

  param->value = strdup(value);
  if (param->value == NULL) {
    SAFE_FREE(param->name);
    oom_error();
    return -1;
  }

  return 0;
}

struct deltacloud_create_parameter *deltacloud_allocate_parameter(const char *name,
								  const char *value)
{
  struct deltacloud_create_parameter *ret;

  ret = malloc(sizeof(struct deltacloud_create_parameter));
  if (ret == NULL) {
    oom_error();
    return NULL;
  }

  if (deltacloud_prepare_parameter(ret, name, value) < 0) {
    /* deltacloud_prepare_parameter set the error already */
    SAFE_FREE(ret);
    return NULL;
  }

  return ret;
}

void deltacloud_free_parameter_value(struct deltacloud_create_parameter *param)
{
  SAFE_FREE(param->name);
  SAFE_FREE(param->value);
}

void deltacloud_free_parameter(struct deltacloud_create_parameter *param)
{
  deltacloud_free_parameter_value(param);
  SAFE_FREE(param);
}

/* this is a function to parse the headers that are returned from a create
 * instance call.  Note that this function *will* change the passed-in
 * headers string
 */
static char *parse_create_headers(char *headers)
{
  char *header;
  char *instid = NULL;
  char *tmp;

  header = strsep(&headers, "\n");
  while (header != NULL) {
    if (strncasecmp(header, "Location:", 9) == 0) {
      tmp = strrchr(header, '/');
      if (tmp == NULL) {
	set_error(DELTACLOUD_NAME_NOT_FOUND_ERROR,
		  "Could not parse instance name after instance creation");
	return NULL;
      }
      tmp++;
      tmp[strlen(tmp) - 1] = '\0';
      instid = strdup(tmp);

      break;
    }
    header = strsep(&headers, "\n");
  }

  if (instid == NULL)
    set_error(DELTACLOUD_NAME_NOT_FOUND_ERROR,
	      "Could not find instance name after instance creation");

  return instid;
}

int deltacloud_create_instance(struct deltacloud_api *api, const char *image_id,
			       struct deltacloud_create_parameter *params,
			       int params_length, char **instance_id)
{
  struct deltacloud_link *thislink;
  size_t param_string_length;
  FILE *paramfp;
  int ret = -1;
  char *data = NULL;
  char *param_string = NULL;
  int i;
  char *safevalue = NULL;
  char *headers = NULL;

  if (!valid_arg(api) || !valid_arg(image_id))
    return -1;

  deltacloud_for_each(thislink, api->links) {
    if (STREQ(thislink->rel, "instances"))
      break;
  }
  if (thislink == NULL) {
    link_error("instances");
    return -1;
  }

  paramfp = open_memstream(&param_string, &param_string_length);
  if (paramfp == NULL) {
    oom_error();
    return -1;
  }

  /* since the parameters come from the user, we must not trust them and
   * URL escape them before use
   */

  safevalue = curl_escape(image_id, 0);
  if (safevalue == NULL) {
      oom_error();
      goto cleanup;
  }
  fprintf(paramfp, "image_id=%s", safevalue);
  SAFE_FREE(safevalue);

  for (i = 0; i < params_length; i++) {
    if (params[i].value != NULL) {
      safevalue = curl_escape(params[i].value, 0);
      if (safevalue == NULL) {
	oom_error();
	goto cleanup;
      }
      fprintf(paramfp, "&%s=%s", params[i].name, safevalue);
      SAFE_FREE(safevalue);
    }
  }

  fclose(paramfp);
  paramfp = NULL;

  if (post_url(thislink->href, api->user, api->password, param_string,
	       &data, &headers) != 0)
    /* post_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (data != NULL && is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_POST_URL_ERROR);
    goto cleanup;
  }

  if (headers == NULL) {
    set_error(DELTACLOUD_POST_URL_ERROR,
	      "No headers returned from create call");
    goto cleanup;
  }

  if (instance_id != NULL) {
    *instance_id = parse_create_headers(headers);
    if (*instance_id == NULL)
      /* parse_create_headers already set the error */
      goto cleanup;
  }

  ret = 0;

 cleanup:
  if (paramfp != NULL)
    fclose(paramfp);
  SAFE_FREE(param_string);
  SAFE_FREE(data);
  SAFE_FREE(headers);

  return ret;
}

static int instance_action(struct deltacloud_api *api,
			   struct deltacloud_instance *instance,
			   const char *action_name)
{
  struct deltacloud_action *act = NULL;
  char *data = NULL;
  int ret = -1;

  if (!valid_arg(api) || !valid_arg(instance))
    return -1;

  /* action_name can't possibly be NULL since it is not part of the
   * external API
   */

  deltacloud_for_each(act, instance->actions) {
    if (STREQ(act->rel, action_name))
      break;
  }
  if (act == NULL) {
    link_error(action_name);
    return -1;
  }

  if (post_url(act->href, api->user, api->password, NULL, &data, NULL) != 0)
    /* post_url sets its own errors, so don't overwrite it here */
    return -1;

  if (data != NULL && is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_POST_URL_ERROR);
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_instance_stop(struct deltacloud_api *api,
			     struct deltacloud_instance *instance)
{
  return instance_action(api, instance, "stop");
}

int deltacloud_instance_reboot(struct deltacloud_api *api,
			       struct deltacloud_instance *instance)
{
  return instance_action(api, instance, "reboot");
}

int deltacloud_instance_start(struct deltacloud_api *api,
			      struct deltacloud_instance *instance)
{
  return instance_action(api, instance, "start");
}

int deltacloud_instance_destroy(struct deltacloud_api *api,
				struct deltacloud_instance *instance)
{
  if (!valid_arg(api) || !valid_arg(instance))
    return -1;

  return internal_destroy(instance->href, api->user, api->password);
}

void deltacloud_free(struct deltacloud_api *api)
{
  if (api == NULL)
    return;

  free_link_list(&api->links);
  SAFE_FREE(api->user);
  SAFE_FREE(api->password);
  SAFE_FREE(api->url);
  SAFE_FREE(api->driver);
  SAFE_FREE(api->version);
  memset(api, 0, sizeof(struct deltacloud_api));
}
