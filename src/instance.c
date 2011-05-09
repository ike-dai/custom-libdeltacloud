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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "instance.h"
#include "curl_action.h"

/* instead of putting these in a header we replicate it here so the header
 * files don't need to include libxml2 headers.  This saves client programs
 * from having to have -I/path/to/libxml2/headers in their build paths.
 */
int parse_addresses_xml(xmlNodePtr root, xmlXPathContextPtr ctxt,
			struct deltacloud_address **addresses);
int parse_actions_xml(xmlNodePtr root, struct deltacloud_action **actions);
int parse_one_hardware_profile(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			       void *output);

static int parse_one_instance(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			      void *output)
{
  struct deltacloud_instance *thisinst = (struct deltacloud_instance *)output;
  xmlXPathObjectPtr hwpset, actionset, pubset, privset;

  memset(thisinst, 0, sizeof(struct deltacloud_instance));

  thisinst->href = (char *)xmlGetProp(cur, BAD_CAST "href");
  thisinst->id = (char *)xmlGetProp(cur, BAD_CAST "id");
  thisinst->name = getXPathString("string(./name)", ctxt);
  thisinst->owner_id = getXPathString("string(./owner_id)", ctxt);
  thisinst->image_id = getXPathString("string(./image/@id)", ctxt);
  thisinst->image_href = getXPathString("string(./image/@href)", ctxt);
  thisinst->realm_id = getXPathString("string(./realm/@id)", ctxt);
  thisinst->realm_href = getXPathString("string(./realm/@href)", ctxt);
  thisinst->state = getXPathString("string(./state)", ctxt);

  hwpset = xmlXPathEval(BAD_CAST "./hardware_profile", ctxt);
  if (hwpset && hwpset->type == XPATH_NODESET && hwpset->nodesetval &&
      hwpset->nodesetval->nodeNr == 1) {
    if (parse_one_hardware_profile(hwpset->nodesetval->nodeTab[0], ctxt,
				   &thisinst->hwp) < 0) {
      deltacloud_free_instance(thisinst);
      xmlXPathFreeObject(hwpset);
      return -1;
    }
  }
  xmlXPathFreeObject(hwpset);

  actionset = xmlXPathEval(BAD_CAST "./actions", ctxt);
  if (actionset && actionset->type == XPATH_NODESET &&
      actionset->nodesetval && actionset->nodesetval->nodeNr == 1) {
    if (parse_actions_xml(actionset->nodesetval->nodeTab[0],
			  &(thisinst->actions)) < 0) {
      deltacloud_free_instance(thisinst);
      xmlXPathFreeObject(actionset);
      return -1;
    }
  }
  xmlXPathFreeObject(actionset);

  pubset = xmlXPathEval(BAD_CAST "./public_addresses", ctxt);
  if (pubset && pubset->type == XPATH_NODESET && pubset->nodesetval &&
      pubset->nodesetval->nodeNr == 1) {
    if (parse_addresses_xml(pubset->nodesetval->nodeTab[0], ctxt,
			    &(thisinst->public_addresses)) < 0) {
      deltacloud_free_instance(thisinst);
      xmlXPathFreeObject(pubset);
      return -1;
    }
  }
  xmlXPathFreeObject(pubset);

  privset = xmlXPathEval(BAD_CAST "./private_addresses", ctxt);
  if (privset && privset->type == XPATH_NODESET && privset->nodesetval &&
      privset->nodesetval->nodeNr == 1) {
    if (parse_addresses_xml(privset->nodesetval->nodeTab[0], ctxt,
			    &(thisinst->private_addresses)) < 0) {
      deltacloud_free_instance(thisinst);
      xmlXPathFreeObject(privset);
      return -1;
    }
  }
  xmlXPathFreeObject(privset);

  return 0;
}

static int parse_instance_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			      void **data)
{
  struct deltacloud_instance **instances = (struct deltacloud_instance **)data;
  struct deltacloud_instance *thisinst;
  xmlNodePtr oldnode;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "instance")) {

      ctxt->node = cur;

      thisinst = calloc(1, sizeof(struct deltacloud_instance));
      if (thisinst == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_instance(cur, ctxt, thisinst) < 0) {
	/* parse_one_instance is expected to have set its own error */
	SAFE_FREE(thisinst);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(instances, struct deltacloud_instance, thisinst);
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

static int instance_action(struct deltacloud_api *api,
			   struct deltacloud_instance *instance,
			   const char *action_name)
{
  struct deltacloud_action *act = NULL;
  char *data = NULL;
  int ret = -1;

  if (!valid_api(api) || !valid_arg(instance))
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

int deltacloud_create_instance(struct deltacloud_api *api, const char *image_id,
			       struct deltacloud_create_parameter *params,
			       int params_length, char **instance_id)
{
  struct deltacloud_create_parameter *internal_params;
  int ret = -1;
  int pos;
  char *headers = NULL;

  if (!valid_api(api) || !valid_arg(image_id))
    return -1;

  if (params_length < 0) {
    invalid_argument_error("params_length must be >= 0");
    return -1;
  }

  internal_params = calloc(params_length + 1,
			   sizeof(struct deltacloud_create_parameter));
  if (internal_params == NULL) {
    oom_error();
    return -1;
  }

  pos = copy_parameters(internal_params, params, params_length);
  if (pos < 0)
    /* copy_parameters already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "image_id",
				   image_id) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (internal_create(api, "instances", internal_params, pos, &headers) < 0)
    /* internal_create already set the error */
    goto cleanup;

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
  free_parameters(internal_params, pos);
  SAFE_FREE(internal_params);
  SAFE_FREE(headers);

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
  if (!valid_api(api) || !valid_arg(instance))
    return -1;

  return internal_destroy(instance->href, api->user, api->password);
}

int deltacloud_get_instances(struct deltacloud_api *api,
			     struct deltacloud_instance **instances)
{
  return internal_get(api, "instances", "instances", parse_instance_xml,
		      (void **)instances);
}

int deltacloud_get_instance_by_id(struct deltacloud_api *api, const char *id,
				  struct deltacloud_instance *instance)
{
  return internal_get_by_id(api, id, "instances", "instance",
			    parse_one_instance, instance);
}

void deltacloud_free_instance(struct deltacloud_instance *instance)
{
  if (instance == NULL)
    return;

  SAFE_FREE(instance->href);
  SAFE_FREE(instance->id);
  SAFE_FREE(instance->name);
  SAFE_FREE(instance->owner_id);
  SAFE_FREE(instance->image_id);
  SAFE_FREE(instance->image_href);
  SAFE_FREE(instance->realm_id);
  SAFE_FREE(instance->realm_href);
  SAFE_FREE(instance->state);
  deltacloud_free_hardware_profile(&instance->hwp);
  free_action_list(&instance->actions);
  free_address_list(&instance->public_addresses);
  free_address_list(&instance->private_addresses);
}

void deltacloud_free_instance_list(struct deltacloud_instance **instances)
{
  free_list(instances, struct deltacloud_instance, deltacloud_free_instance);
}
