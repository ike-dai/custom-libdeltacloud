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
#include "libdeltacloud.h"

static int copy_storage_snapshot(struct deltacloud_storage_snapshot *dst,
				 struct deltacloud_storage_snapshot *src)
{
  /* with a NULL src, we just return success.  A NULL dst is an error */
  if (src == NULL)
    return 0;
  if (dst == NULL)
    return -1;

  memset(dst, 0, sizeof(struct deltacloud_storage_snapshot));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->created, src->created) < 0)
    goto error;
  if (strdup_or_null(&dst->state, src->state) < 0)
    goto error;
  if (strdup_or_null(&dst->storage_volume_href, src->storage_volume_href) < 0)
    goto error;
  if (strdup_or_null(&dst->storage_volume_id, src->storage_volume_id) < 0)
    goto error;

  return 0;

 error:
  deltacloud_free_storage_snapshot(dst);
  return -1;
}

static int parse_storage_snapshot_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data)
{
  struct deltacloud_storage_snapshot **storage_snapshots = (struct deltacloud_storage_snapshot **)data;
  struct deltacloud_storage_snapshot *thissnapshot;
  int ret = -1;
  xmlNodePtr oldnode;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "storage_snapshot")) {

      ctxt->node = cur;

      thissnapshot = calloc(1, sizeof(struct deltacloud_storage_snapshot));
      if (thissnapshot == NULL) {
	oom_error();
	goto cleanup;
      }

      thissnapshot->href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thissnapshot->id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thissnapshot->created = getXPathString("string(./created)", ctxt);
      thissnapshot->state = getXPathString("string(./state)", ctxt);
      thissnapshot->storage_volume_href = getXPathString("string(./storage_volume/@href)",
							 ctxt);
      thissnapshot->storage_volume_id = getXPathString("string(./storage_volume/@id)",
						       ctxt);

      /* add_to_list can't fail */
      add_to_list(storage_snapshots, struct deltacloud_storage_snapshot,
		  thissnapshot);
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

int deltacloud_get_storage_snapshots(struct deltacloud_api *api,
				     struct deltacloud_storage_snapshot **storage_snapshots)
{
  return internal_get(api, "storage_snapshots", "storage_snapshots",
		      parse_storage_snapshot_xml, (void **)storage_snapshots);
}

int deltacloud_get_storage_snapshot_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_storage_snapshot *storage_snapshot)
{
  return internal_get_by_id(api, id, "storage_snapshots",
			    parse_one_storage_snapshot, storage_snapshot);
}

void deltacloud_free_storage_snapshot(struct deltacloud_storage_snapshot *storage_snapshot)
{
  if (storage_snapshot == NULL)
    return;

  SAFE_FREE(storage_snapshot->href);
  SAFE_FREE(storage_snapshot->id);
  SAFE_FREE(storage_snapshot->created);
  SAFE_FREE(storage_snapshot->state);
  SAFE_FREE(storage_snapshot->storage_volume_href);
  SAFE_FREE(storage_snapshot->storage_volume_id);
}

void deltacloud_free_storage_snapshot_list(struct deltacloud_storage_snapshot **storage_snapshots)
{
  free_list(storage_snapshots, struct deltacloud_storage_snapshot,
	    deltacloud_free_storage_snapshot);
}
