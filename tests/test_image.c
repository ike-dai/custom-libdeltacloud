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
#include <unistd.h>
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
  struct deltacloud_api zeroapi;
  struct deltacloud_image image;
  struct deltacloud_image *images = NULL;
  //struct deltacloud_instance instance;
  //char *instid;
  //int timeout;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  memset(&zeroapi, 0, sizeof(struct deltacloud_api));

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }

  /* test out deltacloud_supports_images */
  if (deltacloud_supports_images(NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_images to fail with NULL api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_images(&zeroapi) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_images to fail with uninitialized api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_images(&api)) {

    /* test out deltacloud_get_images */
    if (deltacloud_get_images(NULL, &images) >= 0) {
      fprintf(stderr, "Expected deltacloud_supports_images to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_images(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_images to fail with NULL images, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_images(&zeroapi, &images) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_images to fail with unintialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_images(&api, &images) < 0) {
      fprintf(stderr, "Failed to get_images: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_image_list(images);

    if (images != NULL) {

      /* test out deltacloud_get_image_by_id */
      if (deltacloud_get_image_by_id(NULL, images->id, &image) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_image_by_id to fail with NULL api, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_image_by_id(&api, NULL, &image) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_image_by_id to fail with NULL id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_image_by_id(&api, images->id, NULL) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_image_by_id to fail with NULL image, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_image_by_id(&api, "bogus_id", &image) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_image_by_id to fail with bogus id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_image_by_id(&zeroapi, images->id, &image) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_image_by_id to fail with unintialized api, but succeeded\n");
	goto cleanup;
      }

      /* here we use the first image from the list above */
      if (deltacloud_get_image_by_id(&api, images->id, &image) < 0) {
	fprintf(stderr, "Failed to get image by id: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }
      print_image(&image);
      deltacloud_free_image(&image);
    }


    /* FIXME: due to bugs in deltacloud, I can't seem to make this work
     * at present.  We'll have to revisit.
     */
#if 0
    if (deltacloud_supports_instances(&api)) {

      /* the first thing we need to do is create an instance that we will
       * snapshot from.  In order to create an instance, we need to find
       * an image to base the instance off of.  We fetch the list of images
       * and just use the first one
       */
      if (deltacloud_get_images(&api, &images) < 0) {
	fprintf(stderr, "Failed to get images: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }

      if (deltacloud_create_instance(&api, images->id, NULL, 0, &instid) < 0) {
	deltacloud_free_image_list(&images);
	fprintf(stderr, "Failed to create instance: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }

      deltacloud_free_image_list(&images);

      if (wait_for_instance_boot(api, instid, &instance) != 1) {
	free(instid);
	goto cleanup;
      }

      free(instid);

      /* if we made it here, then the instance went to running.  We can
       * now do operations on it
       */
      if (deltacloud_create_image(&api, instance.id, NULL, 0) < 0) {
	deltacloud_instance_destroy(&api, &instance);
	deltacloud_free_instance(&instance);
	fprintf(stderr, "Failed to create image from instance: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }

      deltacloud_instance_destroy(&api, &instance);
      deltacloud_free_instance(&instance);
    }
    else
      fprintf(stderr, "Instances are not supported, so image creation test is skipped\n");
#endif
  }
  else
    fprintf(stderr, "Images are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free_image_list(&images);

  deltacloud_free(&api);

  return ret;
}
