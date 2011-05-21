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

#ifndef LIBDELTACLOUD_IMAGE_H
#define LIBDELTACLOUD_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing a single deltacloud image.
 */
struct deltacloud_image {
  char *href; /**< The full URL to this image */
  char *id; /**< The ID of this image */
  char *description; /**< The description of this image */
  char *architecture; /**< The architecture of this image */
  char *owner_id; /**< The owner ID of this image */
  char *name; /**< The name of this image */
  char *state; /**< The current state of this image */

  struct deltacloud_image *next;
};

#define deltacloud_supports_images(api) deltacloud_has_link(api, "images")
int deltacloud_get_images(struct deltacloud_api *api,
			  struct deltacloud_image **images);
int deltacloud_get_image_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_image *image);
int deltacloud_create_image(struct deltacloud_api *api, const char *instance_id,
			    struct deltacloud_create_parameter *params,
			    int params_length);
void deltacloud_free_image(struct deltacloud_image *image);
void deltacloud_free_image_list(struct deltacloud_image **images);

#ifdef __cplusplus
}
#endif

#endif
