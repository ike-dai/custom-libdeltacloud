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
#include "image.h"

int add_to_image_list(struct deltacloud_image **images, const char *href, const char *id,
		      const char *description, const char *architecture,
		      const char *owner_id, const char *name)
{
  struct deltacloud_image *oneimage, *curr, *last;

  oneimage = malloc(sizeof(struct deltacloud_image));
  if (oneimage == NULL)
    return -1;

  memset(oneimage, 0, sizeof(struct deltacloud_image));

  if (strdup_or_null(&oneimage->href, href) < 0)
    goto error;
  if (strdup_or_null(&oneimage->id, id) < 0)
    goto error;
  if (strdup_or_null(&oneimage->description, description) < 0)
    goto error;
  if (strdup_or_null(&oneimage->architecture, architecture) < 0)
    goto error;
  if (strdup_or_null(&oneimage->owner_id, owner_id) < 0)
    goto error;
  if (strdup_or_null(&oneimage->name, name) < 0)
    goto error;
  oneimage->next = NULL;

  if (*images == NULL)
    /* First element in the list */
    *images = oneimage;
  else {
    curr = *images;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneimage;
  }

  return 0;

 error:
  deltacloud_free_image(oneimage);
  MY_FREE(oneimage);
  return -1;
}

int copy_image(struct deltacloud_image *dst, struct deltacloud_image *src)
{
  memset(dst, 0, sizeof(struct deltacloud_image));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->description, src->description) < 0)
    goto error;
  if (strdup_or_null(&dst->architecture, src->architecture) < 0)
    goto error;
  if (strdup_or_null(&dst->owner_id, src->owner_id) < 0)
    goto error;
  if (strdup_or_null(&dst->name, src->name) < 0)
    goto error;
  dst->next = NULL;

  return 0;

 error:
  deltacloud_free_image(dst);
  return -1;
}

void deltacloud_print_image(struct deltacloud_image *image, FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "Href: %s\n", image->href);
  fprintf(stream, "ID: %s\n", image->id);
  fprintf(stream, "Description: %s\n", image->description);
  fprintf(stream, "Architecture: %s\n", image->architecture);
  fprintf(stream, "Owner ID: %s\n", image->owner_id);
  fprintf(stream, "Name: %s\n", image->name);
}

void deltacloud_print_image_list(struct deltacloud_image **images, FILE *stream)
{
  struct deltacloud_image *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *images;
  while (curr != NULL) {
    deltacloud_print_image(curr, NULL);
    curr = curr->next;
  }
}

void deltacloud_free_image(struct deltacloud_image *image)
{
  MY_FREE(image->href);
  MY_FREE(image->id);
  MY_FREE(image->description);
  MY_FREE(image->architecture);
  MY_FREE(image->owner_id);
  MY_FREE(image->name);
}

void deltacloud_free_image_list(struct deltacloud_image **images)
{
  struct deltacloud_image *curr, *next;

  curr = *images;
  while (curr != NULL) {
    next = curr->next;
    deltacloud_free_image(curr);
    MY_FREE(curr);
    curr = next;
  }

  *images = NULL;
}
