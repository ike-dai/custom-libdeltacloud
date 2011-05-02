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
  char *name;
  int listret;

  while (featurenode != NULL) {
    if (featurenode->type == XML_ELEMENT_NODE &&
	STREQ((const char *)featurenode->name, "feature")) {
      name = (char *)xmlGetProp(featurenode, BAD_CAST "name");
      listret = add_to_feature_list(features, name);
      SAFE_FREE(name);
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
  xmlNodePtr linknode;
  char *href = NULL, *rel = NULL;
  int ret = -1;
  int listret;
  struct deltacloud_feature *features = NULL;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "api")) {
      (*api)->driver = (char *)xmlGetProp(cur, BAD_CAST "driver");
      (*api)->version = (char *)xmlGetProp(cur, BAD_CAST "version");

      linknode = cur->children;
      while (linknode != NULL) {
	if (linknode->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)linknode->name, "link")) {
	  href = (char *)xmlGetProp(linknode, BAD_CAST "href");
	  if (href == NULL) {
	    xml_error("api", "Failed to parse XML",
		      "did not see href property");
	    goto cleanup;
	  }

	  rel = (char *)xmlGetProp(linknode, BAD_CAST "rel");
	  if (rel == NULL) {
	    xml_error("api", "Failed to parse XML", "did not see rel property");
	    goto cleanup;
	  }

	  if (parse_feature_xml(linknode->children, &features) < 0)
	    /* parse_feature_xml already set the error */
	    goto cleanup;

	  listret = add_to_link_list(&((*api)->links), href, rel, features);
	  SAFE_FREE(href);
	  SAFE_FREE(rel);
	  free_feature_list(&features);
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
  /* in the normal case, these aren't necessary, but they will be for the
   * error case
   */
  SAFE_FREE(href);
  SAFE_FREE(rel);
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

static struct deltacloud_address *parse_addresses_xml(xmlNodePtr instance,
						      xmlXPathContextPtr ctxt)
{
  xmlNodePtr oldnode, cur;
  struct deltacloud_address *addresses = NULL;
  char *address = NULL;
  int failed = 1;
  int listret;

  oldnode = ctxt->node;

  ctxt->node = instance;
  cur = instance->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "address")) {
      address = getXPathString("string(./address)", ctxt);
      if (address != NULL) {
	listret = add_to_address_list(&addresses, address);
	SAFE_FREE(address);
	if (listret < 0) {
	  oom_error();
	  goto cleanup;
	}
      }
      /* address is allowed to be NULL, so skip it here */
    }
    cur = cur->next;
  }

  failed = 0;

 cleanup:
  ctxt->node = oldnode;

  if (failed)
    free_address_list(&addresses);

  return addresses;
}

static struct deltacloud_action *parse_actions_xml(xmlNodePtr instance)
{
  xmlNodePtr cur;
  struct deltacloud_action *actions = NULL;
  char *rel = NULL, *href = NULL, *method = NULL;
  int failed = 1;
  int listret;

  cur = instance->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "link")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("actions", "Failed to parse XML",
		  "did not see href property");
	goto cleanup;
      }

      rel = (char *)xmlGetProp(cur, BAD_CAST "rel");
      if (rel == NULL) {
	xml_error("actions", "Failed to parse XML", "did not see rel property");
	SAFE_FREE(href);
	goto cleanup;
      }

      method = (char *)xmlGetProp(cur, BAD_CAST "method");

      listret = add_to_action_list(&actions, rel, href, method);
      SAFE_FREE(href);
      SAFE_FREE(rel);
      SAFE_FREE(method);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  failed = 0;

 cleanup:
  if (failed)
    free_action_list(&actions);

  return actions;
}

static int parse_hardware_profile_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data);
static int parse_instance_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			      void **data)
{
  struct deltacloud_instance **instances = (struct deltacloud_instance **)data;
  xmlNodePtr oldnode;
  xmlXPathObjectPtr hwpset, actionset, pubset, privset;
  char *href = NULL, *id = NULL, *name = NULL, *owner_id = NULL;
  char *image_id = NULL, *image_href = NULL, *realm_href = NULL;
  char *realm_id = NULL, *state = NULL;
  struct deltacloud_hardware_profile *hwp = NULL;
  struct deltacloud_action *actions = NULL;
  struct deltacloud_address *public_addresses = NULL;
  struct deltacloud_address *private_addresses = NULL;
  int listret;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "instance")) {

      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("instance", "Failed to parse XML",
		  "did not see rel property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("instance", "Failed to parse XML", "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;

      name = getXPathString("string(./name)", ctxt);
      owner_id = getXPathString("string(./owner_id)", ctxt);
      image_id = getXPathString("string(./image/@id)", ctxt);
      image_href = getXPathString("string(./image/@href)", ctxt);
      realm_id = getXPathString("string(./realm/@id)", ctxt);
      realm_href = getXPathString("string(./realm/@href)", ctxt);
      state = getXPathString("string(./state)", ctxt);

      hwpset = xmlXPathEval(BAD_CAST "./hardware_profile", ctxt);
      if (hwpset && hwpset->type == XPATH_NODESET && hwpset->nodesetval &&
	  hwpset->nodesetval->nodeNr == 1)
	parse_hardware_profile_xml(hwpset->nodesetval->nodeTab[0], ctxt,
				   (void **)&hwp);
      xmlXPathFreeObject(hwpset);

      actionset = xmlXPathEval(BAD_CAST "./actions", ctxt);
      if (actionset && actionset->type == XPATH_NODESET &&
	  actionset->nodesetval && actionset->nodesetval->nodeNr == 1)
	actions = parse_actions_xml(actionset->nodesetval->nodeTab[0]);
      xmlXPathFreeObject(actionset);

      pubset = xmlXPathEval(BAD_CAST "./public_addresses", ctxt);
      if (pubset && pubset->type == XPATH_NODESET && pubset->nodesetval &&
	  pubset->nodesetval->nodeNr == 1)
	public_addresses = parse_addresses_xml(pubset->nodesetval->nodeTab[0],
					       ctxt);
      xmlXPathFreeObject(pubset);

      privset = xmlXPathEval(BAD_CAST "./private_addresses", ctxt);
      if (privset && privset->type == XPATH_NODESET && privset->nodesetval &&
	  privset->nodesetval->nodeNr == 1)
	private_addresses = parse_addresses_xml(privset->nodesetval->nodeTab[0],
						ctxt);
      xmlXPathFreeObject(privset);

      listret = add_to_instance_list(instances, href, id, name, owner_id,
				     image_id, image_href, realm_id, realm_href,
				     state, hwp, actions, public_addresses,
				     private_addresses);
      SAFE_FREE(id);
      SAFE_FREE(name);
      SAFE_FREE(owner_id);
      SAFE_FREE(image_id);
      SAFE_FREE(image_href);
      SAFE_FREE(realm_id);
      SAFE_FREE(realm_href);
      SAFE_FREE(state);
      SAFE_FREE(href);
      free_address_list(&public_addresses);
      free_address_list(&private_addresses);
      free_action_list(&actions);
      deltacloud_free_hardware_profile_list(&hwp);
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
  xmlNodePtr oldnode;
  int ret = -1;
  char *href = NULL, *id = NULL, *name = NULL, *state = NULL, *limit = NULL;
  int listret;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "realm")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("realm", "Failed to parse XML", "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("realm", "Failed to parse XML", "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;

      name = getXPathString("string(./name)", ctxt);
      state = getXPathString("string(./state)", ctxt);
      limit = getXPathString("string(./limit)", ctxt);

      listret = add_to_realm_list(realms, href, id, name, state, limit);
      SAFE_FREE(href);
      SAFE_FREE(id);
      SAFE_FREE(name);
      SAFE_FREE(state);
      SAFE_FREE(limit);
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

static struct deltacloud_property_range *parse_hardware_profile_ranges(xmlNodePtr property)
{
  xmlNodePtr prop_cur;
  struct deltacloud_property_range *ranges = NULL;
  struct deltacloud_property_range *ret = NULL;
  char *first = NULL, *last = NULL;
  int listret;

  prop_cur = property->children;

  while (prop_cur != NULL) {
    if (prop_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)prop_cur->name, "range")) {

      first = (char *)xmlGetProp(prop_cur, BAD_CAST "first");
      last = (char *)xmlGetProp(prop_cur, BAD_CAST "last");

      listret = add_to_range_list(&ranges, first, last);
      SAFE_FREE(first);
      SAFE_FREE(last);
      if (listret < 0) {
	oom_error();
	free_range_list(&ranges);
	goto cleanup;
      }
    }

    prop_cur = prop_cur->next;
  }

  ret = ranges;

 cleanup:
  return ret;
}

static struct deltacloud_property_enum *parse_hardware_profile_enums(xmlNodePtr property)
{
  xmlNodePtr prop_cur, enum_cur;
  struct deltacloud_property_enum *enums = NULL;
  struct deltacloud_property_enum *ret = NULL;
  char *enum_value = NULL;
  int listret;

  prop_cur = property->children;

  while (prop_cur != NULL) {
    if (prop_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)prop_cur->name, "enum")) {

      enum_cur = prop_cur->children;
      while (enum_cur != NULL) {
	if (enum_cur->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)enum_cur->name, "entry")) {
	  enum_value = (char *)xmlGetProp(enum_cur, BAD_CAST "value");

	  listret = add_to_enum_list(&enums, enum_value);
	  SAFE_FREE(enum_value);
	  if (listret < 0) {
	    oom_error();
	    free_enum_list(&enums);
	    goto cleanup;
	  }
	}

	enum_cur = enum_cur->next;
      }
    }

    prop_cur = prop_cur->next;
  }

  ret = enums;

 cleanup:
  return ret;
}

static struct deltacloud_property_param *parse_hardware_profile_params(xmlNodePtr property)
{
  xmlNodePtr prop_cur;
  struct deltacloud_property_param *params = NULL;
  struct deltacloud_property_param *ret = NULL;
  char *param_href = NULL, *param_method = NULL, *param_name = NULL;
  char *param_operation = NULL;
  int listret;

  prop_cur = property->children;

  while (prop_cur != NULL) {
    if (prop_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)prop_cur->name, "param")) {

      param_href = (char *)xmlGetProp(prop_cur, BAD_CAST "href");
      param_method = (char *)xmlGetProp(prop_cur, BAD_CAST "method");
      param_name = (char *)xmlGetProp(prop_cur, BAD_CAST "name");
      param_operation = (char *)xmlGetProp(prop_cur, BAD_CAST "operation");

      listret = add_to_param_list(&params, param_href,
				  param_method, param_name,
				  param_operation);
      SAFE_FREE(param_href);
      SAFE_FREE(param_method);
      SAFE_FREE(param_name);
      SAFE_FREE(param_operation);
      if (listret < 0) {
	oom_error();
	free_param_list(&params);
	goto cleanup;
      }
    }
    prop_cur = prop_cur->next;
  }

  ret = params;

 cleanup:
  return ret;
}

static struct deltacloud_property *parse_hardware_profile_properties(xmlNodePtr hwp)
{
  xmlNodePtr profile_cur;
  struct deltacloud_property *props = NULL;
  struct deltacloud_property *ret = NULL;
  struct deltacloud_property_param *params = NULL;
  struct deltacloud_property_enum *enums = NULL;
  struct deltacloud_property_range *ranges = NULL;
  char *kind = NULL, *name = NULL, *unit = NULL, *value = NULL;
  int listret;

  profile_cur = hwp->children;

  while (profile_cur != NULL) {
    if (profile_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)profile_cur->name, "property")) {
      kind = (char *)xmlGetProp(profile_cur, BAD_CAST "kind");
      name = (char *)xmlGetProp(profile_cur, BAD_CAST "name");
      unit = (char *)xmlGetProp(profile_cur, BAD_CAST "unit");
      value = (char *)xmlGetProp(profile_cur, BAD_CAST "value");

      params = parse_hardware_profile_params(profile_cur);
      enums = parse_hardware_profile_enums(profile_cur);
      ranges = parse_hardware_profile_ranges(profile_cur);

      listret = add_to_property_list(&props, kind, name, unit, value, params,
				     enums, ranges);
      SAFE_FREE(kind);
      SAFE_FREE(name);
      SAFE_FREE(unit);
      SAFE_FREE(value);
      free_param_list(&params);
      free_enum_list(&enums);
      free_range_list(&ranges);
      if (listret < 0) {
	oom_error();
	free_property_list(&props);
	goto cleanup;
      }
    }

    profile_cur = profile_cur->next;
  }

  ret = props;

 cleanup:
  return ret;
}

static int parse_hardware_profile_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data)
{
  struct deltacloud_hardware_profile **profiles = (struct deltacloud_hardware_profile **)data;
  struct deltacloud_property *props = NULL;
  char *href = NULL, *id = NULL, *name = NULL;
  xmlNodePtr oldnode;
  int ret = -1;
  int listret;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "hardware_profile")) {
      ctxt->node = cur;

      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("hardware_profile", "Failed to parse XML",
		  "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("hardware_profile", "Failed to parse XML",
		  "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      name = getXPathString("string(./name)", ctxt);

      props = parse_hardware_profile_properties(cur);

      listret = add_to_hardware_profile_list(profiles, id, href, name, props);
      SAFE_FREE(id);
      SAFE_FREE(href);
      SAFE_FREE(name);
      free_property_list(&props);
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
  xmlNodePtr oldnode;
  char *href = NULL, *id = NULL, *description = NULL, *architecture = NULL;
  char *owner_id = NULL, *name = NULL, *state = NULL;
  int listret;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "image")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("image", "Failed to parse XML", "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("image", "Failed to parse XML", "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;
      description = getXPathString("string(./description)", ctxt);
      architecture = getXPathString("string(./architecture)", ctxt);
      owner_id = getXPathString("string(./owner_id)", ctxt);
      name = getXPathString("string(./name)", ctxt);
      state = getXPathString("string(./state)", ctxt);

      listret = add_to_image_list(images, href, id, description, architecture,
				  owner_id, name, state);
      SAFE_FREE(href);
      SAFE_FREE(id);
      SAFE_FREE(description);
      SAFE_FREE(architecture);
      SAFE_FREE(owner_id);
      SAFE_FREE(name);
      SAFE_FREE(state);
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
  xmlNodePtr state_cur;
  char *name = NULL, *action = NULL, *to = NULL, *auto_bool = NULL;
  struct deltacloud_instance_state_transition *transitions = NULL;
  int ret = -1;
  int listret;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "state")) {
      name = (char *)xmlGetProp(cur, BAD_CAST "name");

      state_cur = cur->children;
      while (state_cur != NULL) {
	if (state_cur->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)state_cur->name, "transition")) {
	  action = (char *)xmlGetProp(state_cur, BAD_CAST "action");
	  to = (char *)xmlGetProp(state_cur, BAD_CAST "to");
	  auto_bool = (char *)xmlGetProp(state_cur, BAD_CAST "auto");
	  add_to_transition_list(&transitions, action, to, auto_bool);
	  SAFE_FREE(action);
	  SAFE_FREE(to);
	  SAFE_FREE(auto_bool);
	}
	state_cur = state_cur->next;
      }
      listret = add_to_instance_state_list(instance_states, name, transitions);
      SAFE_FREE(name);
      free_transition_list(&transitions);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
      transitions = NULL;
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
  struct deltacloud_instance_state *found;
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

  found = find_by_name_in_instance_state_list(&statelist, name);
  if (found == NULL) {
    find_by_name_error(name, "instance_state");
    goto cleanup;
  }
  if (copy_instance_state(instance_state, found) < 0) {
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
  xmlNodePtr oldnode;
  int ret = -1;
  char *href = NULL, *id = NULL, *created = NULL, *state = NULL;
  struct deltacloud_storage_volume_capacity capacity;
  char *device = NULL, *instance_href = NULL, *realm_id = NULL;
  struct deltacloud_storage_volume_mount mount;
  int listret;

  memset(&capacity, 0, sizeof(struct deltacloud_storage_volume_capacity));
  memset(&mount, 0, sizeof(struct deltacloud_storage_volume_mount));

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "storage_volume")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("storage_volume", "Failed to parse XML",
		  "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("storage_volume", "Failed to parse XML",
		  "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;
      created = getXPathString("string(./created)", ctxt);
      state = getXPathString("string(./state)", ctxt);
      capacity.unit = getXPathString("string(./capacity/@unit)", ctxt);
      capacity.size = getXPathString("string(./capacity)", ctxt);
      device = getXPathString("string(./device)", ctxt);
      instance_href = getXPathString("string(./instance/@href)", ctxt);
      realm_id = getXPathString("string(./realm_id)", ctxt);
      mount.instance_href = getXPathString("string(./mount/instance/@href)",
					   ctxt);
      mount.instance_id = getXPathString("string(./mount/instance/@id)", ctxt);
      mount.device_name = getXPathString("string(./mount/device/@name)", ctxt);

      listret = add_to_storage_volume_list(storage_volumes, href, id, created,
					   state, &capacity, device,
					   instance_href, realm_id, &mount);
      SAFE_FREE(id);
      SAFE_FREE(created);
      SAFE_FREE(state);
      free_capacity(&capacity);
      SAFE_FREE(device);
      SAFE_FREE(instance_href);
      SAFE_FREE(href);
      SAFE_FREE(realm_id);
      free_mount(&mount);
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
  int ret = -1;
  xmlNodePtr oldnode;
  char *href = NULL, *id = NULL, *created = NULL, *state = NULL;
  char *storage_volume_href = NULL, *storage_volume_id = NULL;
  int listret;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "storage_snapshot")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("storage_snapshot", "Failed to parse XML",
		  "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("storage_snapshot", "Failed to parse XML",
		  "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;
      created = getXPathString("string(./created)", ctxt);
      state = getXPathString("string(./state)", ctxt);
      storage_volume_href = getXPathString("string(./storage_volume/@href)",
					   ctxt);
      storage_volume_id = getXPathString("string(./storage_volume/@id)", ctxt);

      listret = add_to_storage_snapshot_list(storage_snapshots, href, id,
					     created, state,
					     storage_volume_href,
					     storage_volume_id);
      SAFE_FREE(id);
      SAFE_FREE(created);
      SAFE_FREE(state);
      SAFE_FREE(storage_volume_href);
      SAFE_FREE(storage_volume_id);
      SAFE_FREE(href);
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
  int ret = -1;
  char *href = NULL, *id = NULL, *type = NULL, *state = NULL;
  char *fingerprint = NULL;
  xmlNodePtr oldnode;
  int listret;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "key")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("key", "Failed to parse XML", "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("key", "Failed to parse XML", "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      type = (char *)xmlGetProp(cur, BAD_CAST "type");
      if (type == NULL) {
	xml_error("key", "Failed to parse XML", "did not see type property");
	SAFE_FREE(href);
	SAFE_FREE(id);
	goto cleanup;
      }

      ctxt->node = cur;
      state = getXPathString("string(./state)", ctxt);
      fingerprint = getXPathString("string(./fingerprint)", ctxt);

      listret = add_to_key_list(keys, href, id, type, state, fingerprint);
      SAFE_FREE(href);
      SAFE_FREE(id);
      SAFE_FREE(type);
      SAFE_FREE(state);
      SAFE_FREE(fingerprint);
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
  int ret = -1;
  char *href = NULL, *id = NULL, *name = NULL, *provider_id;
  xmlNodePtr oldnode, providernode;
  struct deltacloud_driver_provider *providers = NULL;
  int listret;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "driver")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("key", "Failed to parse XML", "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("key", "Failed to parse XML", "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;
      name = getXPathString("string(./name)", ctxt);

      providernode = cur->children;
      while (providernode != NULL) {
	if (providernode->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)providernode->name, "provider")) {
	  provider_id = (char *)xmlGetProp(providernode, BAD_CAST "id");

	  listret = add_to_provider_list(&providers, provider_id);
	  SAFE_FREE(provider_id);
	  if (listret < 0) {
	    free_provider_list(&providers);
	    oom_error();
	    goto cleanup;
	  }
	}
	providernode = providernode->next;
      }

      listret = add_to_driver_list(drivers, href, id, name, providers);
      SAFE_FREE(href);
      SAFE_FREE(id);
      SAFE_FREE(name);
      free_provider_list(&providers);
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
