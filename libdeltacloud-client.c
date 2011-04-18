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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libdeltacloud.h"

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_instance *instances;
  struct deltacloud_image *images;
  struct deltacloud_realm *realms;
  struct deltacloud_instance_state *instance_states;
  struct deltacloud_storage_volume *storage_volumes;
  struct deltacloud_storage_snapshot *storage_snapshots;
  struct deltacloud_hardware_profile *profiles;
  struct deltacloud_instance_state instance_state;
  struct deltacloud_realm realm;
  struct deltacloud_hardware_profile hwp;
  struct deltacloud_image image;
  struct deltacloud_instance instance;
  struct deltacloud_storage_volume storage_volume;
  struct deltacloud_storage_snapshot storage_snapshot;
  char *instid;
  struct deltacloud_create_parameter stackparams[2];
  struct deltacloud_create_parameter *heapparams[2];
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  fprintf(stderr, "--------------LINKS--------------------------\n");
  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }
  deltacloud_print_link_list(&api.links, NULL);

  fprintf(stderr, "--------------HARDWARE PROFILES--------------\n");
  if (deltacloud_get_hardware_profiles(&api, &profiles) < 0) {
    fprintf(stderr, "Failed to get_hardware_profiles: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }

  deltacloud_print_hardware_profile_list(&profiles, NULL);
  deltacloud_free_hardware_profile_list(&profiles);

  if (deltacloud_get_hardware_profile_by_id(&api, "m1-small", &hwp) < 0) {
    fprintf(stderr, "Failed to get hardware profile by id: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_hardware_profile(&hwp, NULL);
  deltacloud_free_hardware_profile(&hwp);

  fprintf(stderr, "--------------IMAGES-------------------------\n");
  if (deltacloud_get_images(&api, &images) < 0) {
    fprintf(stderr, "Failed to get_images: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_image_list(&images, NULL);
  deltacloud_free_image_list(&images);

  if (deltacloud_get_image_by_id(&api, "img1", &image) < 0) {
    fprintf(stderr, "Failed to get image by id: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_image(&image, NULL);
  deltacloud_free_image(&image);

  fprintf(stderr, "--------------REALMS-------------------------\n");
  if (deltacloud_get_realms(&api, &realms) < 0) {
    fprintf(stderr, "Failed to get_realms: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_realm_list(&realms, NULL);
  deltacloud_free_realm_list(&realms);

  if (deltacloud_get_realm_by_id(&api, "us", &realm) < 0) {
    fprintf(stderr, "Failed to get realm by id: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_realm(&realm, NULL);
  deltacloud_free_realm(&realm);

  fprintf(stderr, "--------------INSTANCE STATES----------------\n");
  if (deltacloud_get_instance_states(&api, &instance_states) < 0) {
    fprintf(stderr, "Failed to get_instance_states: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_instance_state_list(&instance_states, NULL);
  deltacloud_free_instance_state_list(&instance_states);

  if (deltacloud_get_instance_state_by_name(&api, "start",
					    &instance_state) < 0) {
    fprintf(stderr, "Failed to get instance_state: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_instance_state(&instance_state, NULL);
  deltacloud_free_instance_state(&instance_state);

  fprintf(stderr, "--------------INSTANCES---------------------\n");
  if (deltacloud_get_instances(&api, &instances) < 0) {
    fprintf(stderr, "Failed to get_instances: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_instance_list(&instances, NULL);
  deltacloud_free_instance_list(&instances);

  if (deltacloud_get_instance_by_id(&api, "inst1", &instance) < 0) {
    fprintf(stderr, "Failed to get instance by id: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_instance(&instance, NULL);
  deltacloud_free_instance(&instance);

  fprintf(stderr, "--------------STORAGE VOLUMES---------------\n");
  if (deltacloud_get_storage_volumes(&api, &storage_volumes) < 0) {
    fprintf(stderr, "Failed to get_storage_volumes: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_storage_volume_list(&storage_volumes, NULL);
  deltacloud_free_storage_volume_list(&storage_volumes);

  if (deltacloud_get_storage_volume_by_id(&api, "vol3", &storage_volume) < 0) {
    fprintf(stderr, "Failed to get storage volume by ID: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_storage_volume(&storage_volume, NULL);
  deltacloud_free_storage_volume(&storage_volume);

  fprintf(stderr, "--------------STORAGE SNAPSHOTS-------------\n");
  if (deltacloud_get_storage_snapshots(&api, &storage_snapshots) < 0) {
    fprintf(stderr, "Failed to get_storage_snapshots: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_storage_snapshot_list(&storage_snapshots, NULL);
  deltacloud_free_storage_snapshot_list(&storage_snapshots);

  if (deltacloud_get_storage_snapshot_by_id(&api, "snap2",
					    &storage_snapshot) < 0) {
    fprintf(stderr, "Failed to get storage_snapshot by ID: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_print_storage_snapshot(&storage_snapshot, NULL);
  deltacloud_free_storage_snapshot(&storage_snapshot);

  fprintf(stderr, "--------------CREATE INSTANCE PARAMS--------\n");
  if (deltacloud_prepare_parameter(&stackparams[0], "name", "foo") < 0) {
    fprintf(stderr, "Failed to prepare stack parameter 0: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  if (deltacloud_prepare_parameter(&stackparams[1], "keyname", "chris") < 0) {
    deltacloud_free_parameter_value(&stackparams[0]);
    fprintf(stderr, "Failed to prepare stack parameter 1: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_free_parameter_value(&stackparams[0]);
  deltacloud_free_parameter_value(&stackparams[1]);

  heapparams[0] = deltacloud_allocate_parameter("name", "foo");
  if (heapparams[0] == NULL) {
    fprintf(stderr, "Failed to prepare heap parameter 0: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  heapparams[1] = deltacloud_allocate_parameter("keyname", "chris");
  if (heapparams[1] == NULL) {
    deltacloud_free_parameter(heapparams[0]);
    fprintf(stderr, "Failed to prepare heap parameter 1: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_free_parameter(heapparams[0]);
  deltacloud_free_parameter(heapparams[1]);

  fprintf(stderr, "--------------CREATE INSTANCE---------------\n");
  deltacloud_prepare_parameter(&stackparams[0], "name", "foo");
  if (deltacloud_create_instance(&api, "img3", stackparams, 1, &instid) < 0) {
    deltacloud_free_parameter_value(&stackparams[0]);
    fprintf(stderr, "Failed to create_instance: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_free_parameter_value(&stackparams[0]);

  fprintf(stderr, "Successfully created instance with ID %s\n", instid);

  if (deltacloud_get_instance_by_id(&api, instid, &instance) < 0) {
    free(instid);
    fprintf(stderr, "Failed to get instance by id: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }

  free(instid);

  deltacloud_print_instance(&instance, NULL);
  if (deltacloud_instance_stop(&api, &instance) < 0)
    fprintf(stderr, "Failed to stop instance: %s\n",
	    deltacloud_get_last_error_string());
  deltacloud_print_instance(&instance, NULL);
  if (deltacloud_instance_start(&api, &instance) < 0)
    fprintf(stderr, "Failed to start instance: %s\n",
	    deltacloud_get_last_error_string());
  deltacloud_print_instance(&instance, NULL);
  if (deltacloud_instance_reboot(&api, &instance) < 0)
    fprintf(stderr, "Failed to reboot instance: %s\n",
	    deltacloud_get_last_error_string());
  deltacloud_print_instance(&instance, NULL);
  if (deltacloud_instance_destroy(&api, &instance) < 0)
    fprintf(stderr, "Failed to destroy instance: %s\n",
	    deltacloud_get_last_error_string());
  deltacloud_print_instance(&instance, NULL);
  deltacloud_free_instance(&instance);

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
