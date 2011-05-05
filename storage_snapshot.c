/*
 * Copyright (C) 2010 Red Hat, Inc.
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
#include "storage_snapshot.h"

int copy_storage_snapshot(struct deltacloud_storage_snapshot *dst,
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
  dst->next = NULL;

  return 0;

 error:
  deltacloud_free_storage_snapshot(dst);
  return -1;
}

int add_to_storage_snapshot_list(struct deltacloud_storage_snapshot **storage_snapshots,
				 struct deltacloud_storage_snapshot *snapshot)
{
  struct deltacloud_storage_snapshot *onestorage_snapshot;

  onestorage_snapshot = calloc(1, sizeof(struct deltacloud_storage_snapshot));
  if (onestorage_snapshot == NULL)
    return -1;

  if (copy_storage_snapshot(onestorage_snapshot, snapshot) < 0)
    goto error;

  add_to_list(storage_snapshots, struct deltacloud_storage_snapshot,
	      onestorage_snapshot);

  return 0;

 error:
  deltacloud_free_storage_snapshot(onestorage_snapshot);
  SAFE_FREE(onestorage_snapshot);
  return -1;
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
