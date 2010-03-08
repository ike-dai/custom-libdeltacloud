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

int add_to_storage_volume_list(struct deltacloud_storage_volume **storage_volumes,
			       const char *href, const char *id,
			       const char *created, const char *state,
			       const char *capacity, const char *device,
			       const char *instance_href)
{
  struct deltacloud_storage_volume *onestorage_volume, *curr, *last;

  onestorage_volume = malloc(sizeof(struct deltacloud_storage_volume));
  if (onestorage_volume == NULL)
    return -1;

  memset(onestorage_volume, 0, sizeof(struct deltacloud_storage_volume));

  if (strdup_or_null(&onestorage_volume->href, href) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->id, id) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->created, created) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->state, state) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->capacity, capacity) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->device, device) < 0)
    goto error;
  if (strdup_or_null(&onestorage_volume->instance_href, instance_href) < 0)
    goto error;
  onestorage_volume->next = NULL;

  if (*storage_volumes == NULL)
    /* First element in the list */
    *storage_volumes = onestorage_volume;
  else {
    curr = *storage_volumes;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = onestorage_volume;
  }

  return 0;

 error:
  deltacloud_free_storage_volume(onestorage_volume);
  MY_FREE(onestorage_volume);
  return -1;
}

int copy_storage_volume(struct deltacloud_storage_volume *dst,
			struct deltacloud_storage_volume *src)
{
  memset(dst, 0, sizeof(struct deltacloud_storage_volume));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->created, src->created) < 0)
    goto error;
  if (strdup_or_null(&dst->state, src->state) < 0)
    goto error;
  if (strdup_or_null(&dst->capacity, src->capacity) < 0)
    goto error;
  if (strdup_or_null(&dst->device, src->device) < 0)
    goto error;
  if (strdup_or_null(&dst->instance_href, src->instance_href) < 0)
    goto error;
  dst->next = NULL;

  return 0;

 error:
  deltacloud_free_storage_volume(dst);
  return -1;
}

void deltacloud_print_storage_volume(struct deltacloud_storage_volume *storage_volume,
				     FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "Href: %s\n", storage_volume->href);
  fprintf(stream, "ID: %s\n", storage_volume->id);
  fprintf(stream, "Created: %s\n", storage_volume->created);
  fprintf(stream, "State: %s\n", storage_volume->state);
  fprintf(stream, "Capacity: %s\n", storage_volume->capacity);
  fprintf(stream, "Device: %s\n", storage_volume->device);
  fprintf(stream, "Instance Href: %s\n", storage_volume->instance_href);
}

void deltacloud_print_storage_volume_list(struct deltacloud_storage_volume **storage_volumes,
					  FILE *stream)
{
  struct deltacloud_storage_volume *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *storage_volumes;
  while (curr != NULL) {
    deltacloud_print_storage_volume(curr, stream);
    curr = curr->next;
  }
}

void deltacloud_free_storage_volume(struct deltacloud_storage_volume *storage_volume)
{
  MY_FREE(storage_volume->href);
  MY_FREE(storage_volume->id);
  MY_FREE(storage_volume->created);
  MY_FREE(storage_volume->state);
  MY_FREE(storage_volume->capacity);
  MY_FREE(storage_volume->device);
  MY_FREE(storage_volume->instance_href);
}

void deltacloud_free_storage_volume_list(struct deltacloud_storage_volume **storage_volumes)
{
  struct deltacloud_storage_volume *curr, *next;

  curr = *storage_volumes;
  while (curr != NULL) {
    next = curr->next;
    deltacloud_free_storage_volume(curr);
    MY_FREE(curr);
    curr = next;
  }

  *storage_volumes = NULL;
}
