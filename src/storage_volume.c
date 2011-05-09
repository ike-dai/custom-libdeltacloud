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
#include "storage_volume.h"

static void free_capacity(struct deltacloud_storage_volume_capacity *curr)
{
  SAFE_FREE(curr->unit);
  SAFE_FREE(curr->size);
}

static void free_mount(struct deltacloud_storage_volume_mount *curr)
{
  SAFE_FREE(curr->instance_href);
  SAFE_FREE(curr->instance_id);
  SAFE_FREE(curr->device_name);
}

static int parse_one_storage_volume(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void *output)
{
  struct deltacloud_storage_volume *thisvolume = (struct deltacloud_storage_volume *)output;

  memset(thisvolume, 0, sizeof(struct deltacloud_storage_volume));

  thisvolume->href = (char *)xmlGetProp(cur, BAD_CAST "href");
  thisvolume->id = (char *)xmlGetProp(cur, BAD_CAST "id");
  thisvolume->created = getXPathString("string(./created)", ctxt);
  thisvolume->state = getXPathString("string(./state)", ctxt);
  thisvolume->capacity.unit = getXPathString("string(./capacity/@unit)",
					     ctxt);
  thisvolume->capacity.size = getXPathString("string(./capacity)", ctxt);
  thisvolume->device = getXPathString("string(./device)", ctxt);
  thisvolume->instance_href = getXPathString("string(./instance/@href)",
					     ctxt);
  thisvolume->realm_id = getXPathString("string(./realm_id)", ctxt);
  thisvolume->mount.instance_href = getXPathString("string(./mount/instance/@href)",
						   ctxt);
  thisvolume->mount.instance_id = getXPathString("string(./mount/instance/@id)",
						 ctxt);
  thisvolume->mount.device_name = getXPathString("string(./mount/device/@name)",
						 ctxt);

  return 0;
}

static int parse_storage_volume_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void **data)
{
  struct deltacloud_storage_volume **storage_volumes = (struct deltacloud_storage_volume **)data;
  struct deltacloud_storage_volume *thisvolume;
  xmlNodePtr oldnode;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "storage_volume")) {

      ctxt->node = cur;

      thisvolume = calloc(1, sizeof(struct deltacloud_storage_volume));
      if (thisvolume == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_storage_volume(cur, ctxt, thisvolume) < 0) {
	/* parse_one_storage_volume is expected to have set its own error */
	SAFE_FREE(thisvolume);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(storage_volumes, struct deltacloud_storage_volume,
		  thisvolume);
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

int deltacloud_get_storage_volume_by_id(struct deltacloud_api *api,
					const char *id,
					struct deltacloud_storage_volume *storage_volume)
{
  return internal_get_by_id(api, id, "storage_volumes", "storage_volume",
			    parse_one_storage_volume, storage_volume);
}

int deltacloud_create_storage_volume(struct deltacloud_api *api,
				     struct deltacloud_create_parameter *params,
				     int params_length)
{
  if (!valid_api(api))
    return -1;

  if (params_length < 0) {
    invalid_argument_error("params_length must be >= 0");
    return -1;
  }

  if (internal_create(api, "storage_volumes", params, params_length, NULL) < 0)
    /* internal_create already set the error */
    return -1;

  return 0;
}

int deltacloud_storage_volume_destroy(struct deltacloud_api *api,
				      struct deltacloud_storage_volume *storage_volume)
{
  if (!valid_api(api) || !valid_arg(storage_volume))
    return -1;

  return internal_destroy(storage_volume->href, api->user, api->password);
}

int deltacloud_storage_volume_attach(struct deltacloud_api *api,
				     struct deltacloud_storage_volume *storage_volume,
				     const char *instance_id,
				     const char *device,
				     struct deltacloud_create_parameter *params,
				     int params_length)
{
  struct deltacloud_link *thislink;
  struct deltacloud_create_parameter *internal_params;
  char *href;
  int ret = -1;
  int pos;
  int rc;

  if (!valid_api(api) || !valid_arg(storage_volume) ||
      !valid_arg(instance_id) || !valid_arg(device))
    return -1;

  if (params_length < 0) {
    invalid_argument_error("params_length must be >= 0");
    return -1;
  }

  deltacloud_for_each(thislink, api->links) {
    if (STREQ(thislink->rel, "storage_volumes"))
      break;
  }
  if (thislink == NULL) {
    link_error("storage_volumes");
    return -1;
  }

  internal_params = calloc(params_length + 3,
			   sizeof(struct deltacloud_create_parameter));
  if (internal_params == NULL) {
    oom_error();
    return -1;
  }

  pos = copy_parameters(internal_params, params, params_length);
  if (pos < 0)
    /* copy_parameters already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "id",
				   storage_volume->id) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "instance_id",
				   instance_id) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "device",
				   device) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (asprintf(&href, "%s/%s/attach", thislink->href, storage_volume->id) < 0) {
    oom_error();
    goto cleanup;
  }

  rc = internal_post(api, href, internal_params, pos, NULL);
  SAFE_FREE(href);
  if (rc < 0)
    /* internal_post already set the error */
    goto cleanup;

  ret = 0;

 cleanup:
  free_parameters(internal_params, pos);
  SAFE_FREE(internal_params);

  return ret;
}

int deltacloud_storage_volume_detach(struct deltacloud_api *api,
				     struct deltacloud_storage_volume *storage_volume,
				     struct deltacloud_create_parameter *params,
				     int params_length)
{
  struct deltacloud_link *thislink;
  char *href;
  int rc;

  if (!valid_api(api) || !valid_arg(storage_volume))
    return -1;

  if (params_length < 0) {
    invalid_argument_error("params_length must be >= 0");
    return -1;
  }

  deltacloud_for_each(thislink, api->links) {
    if (STREQ(thislink->rel, "storage_volumes"))
      break;
  }
  if (thislink == NULL) {
    link_error("storage_volumes");
    return -1;
  }

  if (asprintf(&href, "%s/%s/detach", thislink->href, storage_volume->id) < 0) {
    oom_error();
    return -1;
  }

  rc = internal_post(api, href, params, params_length, NULL);
  SAFE_FREE(href);
  if (rc < 0)
    /* internal_post already set the error */
    return -1;

  return 0;
}

void deltacloud_free_storage_volume(struct deltacloud_storage_volume *storage_volume)
{
  if (storage_volume == NULL)
    return;

  SAFE_FREE(storage_volume->href);
  SAFE_FREE(storage_volume->id);
  SAFE_FREE(storage_volume->created);
  SAFE_FREE(storage_volume->state);
  free_capacity(&storage_volume->capacity);
  SAFE_FREE(storage_volume->device);
  SAFE_FREE(storage_volume->instance_href);
  SAFE_FREE(storage_volume->realm_id);
  free_mount(&storage_volume->mount);
}

void deltacloud_free_storage_volume_list(struct deltacloud_storage_volume **storage_volumes)
{
  free_list(storage_volumes, struct deltacloud_storage_volume,
	    deltacloud_free_storage_volume);
}
