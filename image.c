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

int copy_image(struct deltacloud_image *dst, struct deltacloud_image *src)
{
  /* with a NULL src, we just return success.  A NULL dst is an error */
  if (src == NULL)
    return 0;
  if (dst == NULL)
    return -1;

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
  if (strdup_or_null(&dst->state, src->state) < 0)
    goto error;

  return 0;

 error:
  deltacloud_free_image(dst);
  return -1;
}

int add_to_image_list(struct deltacloud_image **images,
		      struct deltacloud_image *image)
{
  struct deltacloud_image *oneimage;

  oneimage = calloc(1, sizeof(struct deltacloud_image));
  if (oneimage == NULL)
    return -1;

  if (copy_image(oneimage, image) < 0)
    goto error;

  add_to_list(images, struct deltacloud_image, oneimage);

  return 0;

 error:
  deltacloud_free_image(oneimage);
  SAFE_FREE(oneimage);
  return -1;
}

void deltacloud_free_image(struct deltacloud_image *image)
{
  if (image == NULL)
    return;

  SAFE_FREE(image->href);
  SAFE_FREE(image->id);
  SAFE_FREE(image->description);
  SAFE_FREE(image->architecture);
  SAFE_FREE(image->owner_id);
  SAFE_FREE(image->name);
  SAFE_FREE(image->state);
}

void deltacloud_free_image_list(struct deltacloud_image **images)
{
  free_list(images, struct deltacloud_image, deltacloud_free_image);
}
