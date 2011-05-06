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
  fprintf(stderr, "\tKey Name: %s, State: %s\n", instance->key_name,
	  instance->state);
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
  struct deltacloud_instance *instances;
  struct deltacloud_instance instance;
  struct deltacloud_create_parameter stackparams[2];
  char *instid;
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

  if (deltacloud_supports_instances(&api)) {
    if (deltacloud_get_instances(&api, &instances) < 0) {
      fprintf(stderr, "Failed to get_instances: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_instance_list(instances);

    if (instances != NULL) {
      /* here we use the first instance from the list above */
      if (deltacloud_get_instance_by_id(&api, instances->id, &instance) < 0) {
	fprintf(stderr, "Failed to get instance by id: %s\n",
		deltacloud_get_last_error_string());
	deltacloud_free_instance_list(&instances);
	goto cleanup;
      }
      print_instance(&instance);
      deltacloud_free_instance(&instance);
    }

    deltacloud_free_instance_list(&instances);

    deltacloud_prepare_parameter(&stackparams[0], "name", "foo");
    if (deltacloud_create_instance(&api, "img3", stackparams, 1, &instid) < 0) {
      deltacloud_free_parameter_value(&stackparams[0]);
      fprintf(stderr, "Failed to create_instance: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    deltacloud_free_parameter_value(&stackparams[0]);

    if (deltacloud_get_instance_by_id(&api, instid, &instance) < 0) {
      free(instid);
      fprintf(stderr, "Failed to get instance by id: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    free(instid);

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
  deltacloud_free(&api);

  return ret;
}
