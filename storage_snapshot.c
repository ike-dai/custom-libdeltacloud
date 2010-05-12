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

int add_to_storage_snapshot_list(struct deltacloud_storage_snapshot **storage_snapshots,
				 const char *href, const char *id,
				 const char *created, const char *state,
				 const char *storage_volume_href)
{
  struct deltacloud_storage_snapshot *onestorage_snapshot, *curr, *last;

  onestorage_snapshot = malloc(sizeof(struct deltacloud_storage_snapshot));
  if (onestorage_snapshot == NULL)
    return -1;

  memset(onestorage_snapshot, 0, sizeof(struct deltacloud_storage_snapshot));

  if (strdup_or_null(&onestorage_snapshot->href, href) < 0)
    goto error;
  if (strdup_or_null(&onestorage_snapshot->id, id) < 0)
    goto error;
  if (strdup_or_null(&onestorage_snapshot->created, created) < 0)
    goto error;
  if (strdup_or_null(&onestorage_snapshot->state, state) < 0)
    goto error;
  if (strdup_or_null(&onestorage_snapshot->storage_volume_href, storage_volume_href) < 0)
    goto error;
  onestorage_snapshot->next = NULL;

  if (*storage_snapshots == NULL)
    /* First element in the list */
    *storage_snapshots = onestorage_snapshot;
  else {
    curr = *storage_snapshots;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = onestorage_snapshot;
  }

  return 0;

 error:
  deltacloud_free_storage_snapshot(onestorage_snapshot);
  SAFE_FREE(onestorage_snapshot);
  return -1;
}

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
  dst->next = NULL;

  return 0;

 error:
  deltacloud_free_storage_snapshot(dst);
  return -1;
}

void deltacloud_print_storage_snapshot(struct deltacloud_storage_snapshot *storage_snapshot,
				       FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "Href: %s\n", storage_snapshot->href);
  fprintf(stream, "ID: %s\n", storage_snapshot->id);
  fprintf(stream, "Created: %s\n", storage_snapshot->created);
  fprintf(stream, "State: %s\n", storage_snapshot->state);
  fprintf(stream, "Storage Volume Href: %s\n",
	  storage_snapshot->storage_volume_href);
}

void deltacloud_print_storage_snapshot_list(struct deltacloud_storage_snapshot **storage_snapshots,
					    FILE *stream)
{
  struct deltacloud_storage_snapshot *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *storage_snapshots;
  while (curr != NULL) {
    deltacloud_print_storage_snapshot(curr, NULL);
    curr = curr->next;
  }
}

void deltacloud_free_storage_snapshot(struct deltacloud_storage_snapshot *storage_snapshot)
{
  SAFE_FREE(storage_snapshot->href);
  SAFE_FREE(storage_snapshot->id);
  SAFE_FREE(storage_snapshot->created);
  SAFE_FREE(storage_snapshot->state);
  SAFE_FREE(storage_snapshot->storage_volume_href);
}

void deltacloud_free_storage_snapshot_list(struct deltacloud_storage_snapshot **storage_snapshots)
{
  struct deltacloud_storage_snapshot *curr, *next;

  curr = *storage_snapshots;
  while (curr != NULL) {
    next = curr->next;
    deltacloud_free_storage_snapshot(curr);
    SAFE_FREE(curr);
    curr = next;
  }

  *storage_snapshots = NULL;
}
