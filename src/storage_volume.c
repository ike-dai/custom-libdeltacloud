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

static int copy_capacity(struct deltacloud_storage_volume_capacity *dst,
			 struct deltacloud_storage_volume_capacity *src)
{
  memset(dst, 0, sizeof(struct deltacloud_storage_volume_capacity));

  if (strdup_or_null(&dst->unit, src->unit) < 0)
    goto error;
  if (strdup_or_null(&dst->size, src->size) < 0)
    goto error;

  return 0;

 error:
  free_capacity(dst);
  return -1;
}

static void free_mount(struct deltacloud_storage_volume_mount *curr)
{
  SAFE_FREE(curr->instance_href);
  SAFE_FREE(curr->instance_id);
  SAFE_FREE(curr->device_name);
}

static int copy_mount(struct deltacloud_storage_volume_mount *dst,
		      struct deltacloud_storage_volume_mount *src)
{
  memset (dst, 0, sizeof(struct deltacloud_storage_volume_mount));

  if (strdup_or_null(&dst->instance_href, src->instance_href) < 0)
    goto error;
  if (strdup_or_null(&dst->instance_id, src->instance_id) < 0)
    goto error;
  if (strdup_or_null(&dst->device_name, src->device_name) < 0)
    goto error;

  return 0;

 error:
  free_mount(dst);
  return -1;
}

static int copy_storage_volume(struct deltacloud_storage_volume *dst,
			       struct deltacloud_storage_volume *src)
{
  /* with a NULL src, we just return success.  A NULL dst is an error */
  if (src == NULL)
    return 0;
  if (dst == NULL)
    return -1;

  memset(dst, 0, sizeof(struct deltacloud_storage_volume));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->created, src->created) < 0)
    goto error;
  if (strdup_or_null(&dst->state, src->state) < 0)
    goto error;
  if (copy_capacity(&dst->capacity, &src->capacity) < 0)
    goto error;
  if (strdup_or_null(&dst->device, src->device) < 0)
    goto error;
  if (strdup_or_null(&dst->instance_href, src->instance_href) < 0)
    goto error;
  if (strdup_or_null(&dst->realm_id, src->realm_id) < 0)
    goto error;
  if (copy_mount(&dst->mount, &src->mount) < 0)
    goto error;

  return 0;

 error:
  deltacloud_free_storage_volume(dst);
  return -1;
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
  return internal_get_by_id(api, id, "storage_volumes",
			    parse_one_storage_volume, storage_volume);
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
