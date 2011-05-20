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

static void print_instance(struct deltacloud_instance *instance)
{
  fprintf(stderr, "Instance: %s\n", instance->name);
  fprintf(stderr, "\tHref: %s, ID: %s\n", instance->href, instance->id);
  fprintf(stderr, "\tOwner ID: %s\n", instance->owner_id);
  fprintf(stderr, "\tImage ID: %s, Image Href: %s\n", instance->image_id,
	  instance->image_href);
  fprintf(stderr, "\tRealm ID: %s, Realm Href: %s\n", instance->realm_id,
	  instance->realm_href);
  fprintf(stderr, "\tState: %s, Launch Time: %s\n", instance->state,
	  instance->launch_time);
  fprintf(stderr, "\t");
  print_hwp(&instance->hwp);
  print_action_list(instance->actions);
  print_address_list("Public Addresses", instance->public_addresses);
  print_address_list("Private Addresses", instance->private_addresses);
}

static void print_instance_list(struct deltacloud_instance *instances)
{
  struct deltacloud_instance *instance;

  deltacloud_for_each(instance, instances)
    print_instance(instance);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_api zeroapi;
  struct deltacloud_instance *instances = NULL;
  struct deltacloud_instance instance;
  struct deltacloud_image *images = NULL;
  struct deltacloud_create_parameter stackparams[2];
  char *instid;
  int ret = 3;
  int rc;

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

  /* test out deltacloud_supports_instances */
  if (deltacloud_supports_instances(NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_instances to fail with NULL api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_instances(&zeroapi) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_instances to fail with uninitialized api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_instances(&api)) {

    /* test out deltacloud_get_instances */
    if (deltacloud_get_instances(NULL, &instances) >= 0) {
      fprintf(stderr, "Expected deltacloud_supports_instances to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_instances(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_instances to fail with NULL instances, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_instances(&zeroapi, &instances) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_instances to fail with unintialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_instances(&api, &instances) < 0) {
      fprintf(stderr, "Failed to get_instances: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_instance_list(instances);

    if (instances != NULL) {

      /* test out deltacloud_get_instance_by_id */
      if (deltacloud_get_instance_by_id(NULL, instances->id, &instance) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_instance_by_id to fail with NULL api, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_instance_by_id(&api, NULL, &instance) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_instance_by_id to fail with NULL id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_instance_by_id(&api, instances->id, NULL) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_instance_by_id to fail with NULL instance, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_instance_by_id(&api, "bogus_id", &instance) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_instance_by_id to fail with bogus id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_instance_by_id(&zeroapi, instances->id,
					&instance) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_instance_by_id to fail with unintialized api, but succeeded\n");
	goto cleanup;
      }

      /* here we use the first instance from the list above */
      if (deltacloud_get_instance_by_id(&api, instances->id, &instance) < 0) {
	fprintf(stderr, "Failed to get instance by id: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }
      print_instance(&instance);
      deltacloud_free_instance(&instance);

      /* test out deltacloud_get_instance_by_name */
      if (deltacloud_get_instance_by_name(NULL, instances->name,
					  &instance) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_instance_by_name to fail with NULL api, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_instance_by_name(&api, NULL, &instance) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_instance_by_name to fail with NULL id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_instance_by_name(&api, instances->name, NULL) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_instance_by_name to fail with NULL instance, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_instance_by_name(&api, "bogus_id", &instance) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_instance_by_name to fail with bogus id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_instance_by_name(&zeroapi, instances->name,
					  &instance) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_instance_by_name to fail with unintialized api, but succeeded\n");
	goto cleanup;
      }

      /* here we use the first instance from the list above */
      fprintf(stderr, "Going to try to get name %s\n", instances->name);
      if (deltacloud_get_instance_by_name(&api, instances->name,
					  &instance) < 0) {
	fprintf(stderr, "Failed to get instance by name: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }
      print_instance(&instance);
      deltacloud_free_instance(&instance);
    }

    /* in order to create an instance, we need to find an image to use */
    if (deltacloud_get_images(&api, &images) < 0) {
      fprintf(stderr, "Failed to get images: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    if (deltacloud_create_instance(NULL, images->id, NULL, 0, &instid) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_instance to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_instance(&zeroapi, images->id, NULL, 0,
				   &instid) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_instance to fail with uninitialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_instance(&api, NULL, NULL, 0, &instid) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_instance to fail with NULL image id, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_instance(&api, images->id, NULL, -1, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_instance to fail with negative params_length, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_instance(&api, images->id, NULL, 0, NULL) < 0) {
      fprintf(stderr, "Failed to create instance with NULL instid: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    /* FIXME: unfortunately, since we didn't pass a handle into
     * deltacloud_create_instance, we don't know how to destroy it
     */

    if (deltacloud_create_instance(&api, images->id, NULL, 0, &instid) < 0) {
      fprintf(stderr, "Failed to create instance with NULL params: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    rc = wait_for_instance_boot(&api, instid, &instance);
    free(instid);
    if (rc != 1)
      goto cleanup;

    rc = deltacloud_instance_stop(&api, &instance);
    deltacloud_free_instance(&instance);
    if (rc < 0) {
      fprintf(stderr, "Failed to stop instance after creation: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    deltacloud_prepare_parameter(&stackparams[0], "name", "foo");
    rc = deltacloud_create_instance(&api, images->id, stackparams, 1, &instid);
    deltacloud_free_parameter_value(&stackparams[0]);
    if (rc < 0) {
      fprintf(stderr, "Failed to create_instance: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    rc = wait_for_instance_boot(&api, instid, &instance);
    free(instid);
    if (rc != 1) {
      fprintf(stderr, "Failed to get instance by id: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    print_instance(&instance);

    if (deltacloud_instance_stop(&api, &instance) < 0)
      fprintf(stderr, "Failed to stop instance: %s\n",
	      deltacloud_get_last_error_string());
    if (deltacloud_instance_start(&api, &instance) < 0)
      fprintf(stderr, "Failed to start instance: %s\n",
	      deltacloud_get_last_error_string());
    if (deltacloud_instance_reboot(&api, &instance) < 0)
      fprintf(stderr, "Failed to reboot instance: %s\n",
	      deltacloud_get_last_error_string());
    if (deltacloud_instance_destroy(&api, &instance) < 0)
      fprintf(stderr, "Failed to destroy instance: %s\n",
	      deltacloud_get_last_error_string());
    deltacloud_free_instance(&instance);
  }
  else
    fprintf(stderr, "Instances are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free_image_list(&images);
  deltacloud_free_instance_list(&instances);

  deltacloud_free(&api);

  return ret;
}
