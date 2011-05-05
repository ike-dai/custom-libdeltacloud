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
#include "storage_volume.h"

void free_capacity(struct deltacloud_storage_volume_capacity *curr)
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

void free_mount(struct deltacloud_storage_volume_mount *curr)
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

int add_to_storage_volume_list(struct deltacloud_storage_volume **storage_volumes,
			       const char *href, const char *id,
			       const char *created, const char *state,
			       struct deltacloud_storage_volume_capacity *capacity,
			       const char *device, const char *instance_href,
			       const char *realm_id,
			       struct deltacloud_storage_volume_mount *mount)
{
  struct deltacloud_storage_volume *onestorage_volume;

  onestorage_volume = calloc(1, sizeof(struct deltacloud_storage_volume));
  if (onestorage_volume == NULL)
    return -1;

  if (strdup_or_null(&onestorage_volume->href, href) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->id, id) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->created, created) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->state, state) < 0)
    goto error;
  if (copy_capacity(&onestorage_volume->capacity, capacity) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->device, device) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->instance_href, instance_href) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->realm_id, realm_id) < 0)
    goto error;
  if (copy_mount(&onestorage_volume->mount, mount) < 0)
    goto error;

  add_to_list(storage_volumes, struct deltacloud_storage_volume,
	      onestorage_volume);

  return 0;

 error:
  deltacloud_free_storage_volume(onestorage_volume);
  SAFE_FREE(onestorage_volume);
  return -1;
}

int copy_storage_volume(struct deltacloud_storage_volume *dst,
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
  dst->next = NULL;

  return 0;

 error:
  deltacloud_free_storage_volume(dst);
  return -1;
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
