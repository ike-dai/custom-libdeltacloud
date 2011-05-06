/*
 * Copyright (C) 2011 Red Hat, Inc.
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
#include "libdeltacloud.h"
#include "test_common.h"

static void print_image(struct deltacloud_image *image)
{
  fprintf(stderr, "Image: %s\n", image->name);
  fprintf(stderr, "\tID: %s\n", image->id);
  fprintf(stderr, "\tHref: %s\n", image->href);
  fprintf(stderr, "\tDescription: %s\n", image->description);
  fprintf(stderr, "\tOwner ID: %s\n", image->owner_id);
  fprintf(stderr, "\tState: %s\n", image->state);
}

static void print_image_list(struct deltacloud_image *images)
{
  struct deltacloud_image *image;

  deltacloud_for_each(image, images)
    print_image(image);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_image image;
  struct deltacloud_image *images;
  struct deltacloud_instance *instances;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }

  if (deltacloud_supports_images(&api)) {
    if (deltacloud_get_images(&api, &images) < 0) {
      fprintf(stderr, "Failed to get_images: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_image_list(images);

    if (images != NULL) {
      /* here we use the first image from the list above */
      if (deltacloud_get_image_by_id(&api, images->id, &image) < 0) {
	fprintf(stderr, "Failed to get image by id: %s\n",
		deltacloud_get_last_error_string());
	deltacloud_free_image_list(&images);
	goto cleanup;
      }
      print_image(&image);
      deltacloud_free_image(&image);
    }

    deltacloud_free_image_list(&images);

    /* in order to test out image creation, we need to find an active
     * instance
     */
    if (deltacloud_supports_instances(&api)) {
      /* FIXME: instead of looking for an instance, we should control our
       * own destiny and create one.  However, we might have to wait around
       * for it to go running before we do that
       */
      if (deltacloud_get_instances(&api, &instances) < 0) {
	fprintf(stderr, "Failed to get_instances: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }

      if (instances != NULL) {
	if (deltacloud_create_image(&api, instances->id, NULL, 0) < 0) {
	  deltacloud_free_instance_list(&instances);
	  fprintf(stderr, "Failed to create image from instance: %s\n",
		  deltacloud_get_last_error_string());
	  goto cleanup;
	}

	/* FIXME: now that the image has been created, we should look it up
	 * and then delete it
	 */

	deltacloud_free_instance_list(&instances);
      }
    }
  }
  else
    fprintf(stderr, "Images are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
