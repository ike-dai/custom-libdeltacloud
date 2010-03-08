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

#ifndef IMAGE_H
#define IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_image {
  char *href;
  char *id;
  char *description;
  char *architecture;
  char *owner_id;
  char *name;

  struct deltacloud_image *next;
};

int add_to_image_list(struct deltacloud_image **images, const char *href, const char *id,
		      const char *description, const char *architecture,
		      const char *owner_id, const char *name);
int copy_image(struct deltacloud_image *dst, struct deltacloud_image *src);
void deltacloud_print_image(struct deltacloud_image *image, FILE *stream);
void deltacloud_print_image_list(struct deltacloud_image **images, FILE *stream);
void deltacloud_free_image(struct deltacloud_image *image);
void deltacloud_free_image_list(struct deltacloud_image **images);

#ifdef __cplusplus
}
#endif

#endif
