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
#include <string.h>
#include "common.h"
#include "flavor.h"

int copy_flavor(struct deltacloud_flavor *dst, struct deltacloud_flavor *src)
{
  memset(dst, 0, sizeof(struct deltacloud_flavor));

  if (strdup_or_null(&dst->href, src->href) < 0)
    return -1;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->memory, src->memory) < 0)
    goto error;
  if (strdup_or_null(&dst->storage, src->storage) < 0)
    goto error;
  if (strdup_or_null(&dst->architecture, src->architecture) < 0)
    goto error;
  dst->next = NULL;

  return 0;

 error:
  free_flavor(dst);
  return -1;
}

int add_to_flavor_list(struct deltacloud_flavor **flavors, const char *href,
		       const char *id, const char *memory, const char *storage,
		       const char *architecture)
{
  struct deltacloud_flavor *oneflavor, *curr, *last;

  oneflavor = malloc(sizeof(struct deltacloud_flavor));
  if (oneflavor == NULL)
    return -1;

  memset(oneflavor, 0, sizeof(struct deltacloud_flavor));

  if (strdup_or_null(&oneflavor->href, href) < 0)
    goto error;
  if (strdup_or_null(&oneflavor->id, id) < 0)
    goto error;
  if (strdup_or_null(&oneflavor->memory, memory) < 0)
    goto error;
  if (strdup_or_null(&oneflavor->storage, storage) < 0)
    goto error;
  if (strdup_or_null(&oneflavor->architecture, architecture) < 0)
    goto error;
  oneflavor->next = NULL;

  if (*flavors == NULL)
    /* First element in the list */
    *flavors = oneflavor;
  else {
    curr = *flavors;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneflavor;
  }

  return 0;

 error:
  free_flavor(oneflavor);
  MY_FREE(oneflavor);
  return -1;
}

void print_flavor(struct deltacloud_flavor *flavor, FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "Href: %s\n", flavor->href);
  fprintf(stream, "ID: %s\n", flavor->id);
  fprintf(stream, "Memory: %s\n", flavor->memory);
  fprintf(stream, "Storage: %s\n", flavor->storage);
  fprintf(stream, "Architecture: %s\n", flavor->architecture);
}

void print_flavor_list(struct deltacloud_flavor **flavors, FILE *stream)
{
  struct deltacloud_flavor *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *flavors;
  while (curr != NULL) {
    print_flavor(curr, stream);
    curr = curr->next;
  }
}

void free_flavor(struct deltacloud_flavor *flavor)
{
  MY_FREE(flavor->href);
  MY_FREE(flavor->id);
  MY_FREE(flavor->memory);
  MY_FREE(flavor->storage);
  MY_FREE(flavor->architecture);
}

void free_flavor_list(struct deltacloud_flavor **flavors)
{
  struct deltacloud_flavor *curr, *next;

  curr = *flavors;
  while (curr != NULL) {
    next = curr->next;
    free_flavor(curr);
    MY_FREE(curr);
    curr = next;
  }

  *flavors = NULL;
}
