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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dcloudapi.h"

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
  struct deltacloud_instance newinstance;
  int ret = 3;
  int rc;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  fprintf(stderr, "--------------LINKS--------------------------\n");
  rc = deltacloud_initialize(&api, argv[1], argv[2], argv[3]);
  if (rc < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_strerror(rc));
    return 2;
  }
  deltacloud_print_link_list(&api.links, NULL);

  fprintf(stderr, "--------------HARDWARE PROFILES--------------\n");
  rc = deltacloud_get_hardware_profiles(&api, &profiles);
  if (rc < 0) {
    fprintf(stderr, "Failed to get_hardware_profiles: %s\n",
	    deltacloud_strerror(rc));
    goto cleanup;
  }

  deltacloud_print_hardware_profile_list(&profiles, NULL);
  deltacloud_free_hardware_profile_list(&profiles);

  rc = deltacloud_get_hardware_profile_by_id(&api, "m1-small", &hwp);
  if (rc < 0) {
    fprintf(stderr, "Failed to get hardware profile by id: %s\n",
	    deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_hardware_profile(&hwp, NULL);
  deltacloud_free_hardware_profile(&hwp);

  fprintf(stderr, "--------------IMAGES-------------------------\n");
  rc = deltacloud_get_images(&api, &images);
  if (rc < 0) {
    fprintf(stderr, "Failed to get_images: %s\n", deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_image_list(&images, NULL);
  deltacloud_free_image_list(&images);

  rc = deltacloud_get_image_by_id(&api, "img1", &image);
  if (rc < 0) {
    fprintf(stderr, "Failed to get image by id: %s\n", deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_image(&image, NULL);
  deltacloud_free_image(&image);

  fprintf(stderr, "--------------REALMS-------------------------\n");
  rc = deltacloud_get_realms(&api, &realms);
  if (rc < 0) {
    fprintf(stderr, "Failed to get_realms: %s\n", deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_realm_list(&realms, NULL);
  deltacloud_free_realm_list(&realms);

  rc = deltacloud_get_realm_by_id(&api, "us", &realm);
  if (rc < 0) {
    fprintf(stderr, "Failed to get realm by id: %s\n", deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_realm(&realm, NULL);
  deltacloud_free_realm(&realm);

  fprintf(stderr, "--------------INSTANCE STATES----------------\n");
  rc = deltacloud_get_instance_states(&api, &instance_states);
  if (rc < 0) {
    fprintf(stderr, "Failed to get_instance_states: %s\n",
	    deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_instance_state_list(&instance_states, NULL);
  deltacloud_free_instance_state_list(&instance_states);

  rc = deltacloud_get_instance_state_by_name(&api, "start", &instance_state);
  if (rc < 0) {
    fprintf(stderr, "Failed to get instance_state: %s\n",
	    deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_instance_state(&instance_state, NULL);
  deltacloud_free_instance_state(&instance_state);

  fprintf(stderr, "--------------INSTANCES---------------------\n");
  rc = deltacloud_get_instances(&api, &instances);
  if (rc < 0) {
    fprintf(stderr, "Failed to get_instances: %s\n", deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_instance_list(&instances, NULL);
  deltacloud_free_instance_list(&instances);

  rc = deltacloud_get_instance_by_id(&api, "inst1", &instance);
  if (rc < 0) {
    fprintf(stderr, "Failed to get instance by id: %s\n",
	    deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_instance(&instance, NULL);
  deltacloud_free_instance(&instance);

  fprintf(stderr, "--------------STORAGE VOLUMES---------------\n");
  rc = deltacloud_get_storage_volumes(&api, &storage_volumes);
  if (rc < 0) {
    fprintf(stderr, "Failed to get_storage_volumes: %s\n",
	    deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_storage_volume_list(&storage_volumes, NULL);
  deltacloud_free_storage_volume_list(&storage_volumes);

  rc = deltacloud_get_storage_volume_by_id(&api, "vol3", &storage_volume);
  if (rc < 0) {
    fprintf(stderr, "Failed to get storage volume by ID: %s\n",
	    deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_storage_volume(&storage_volume, NULL);
  deltacloud_free_storage_volume(&storage_volume);

  fprintf(stderr, "--------------STORAGE SNAPSHOTS-------------\n");
  rc = deltacloud_get_storage_snapshots(&api, &storage_snapshots);
  if (rc < 0) {
    fprintf(stderr, "Failed to get_storage_snapshots: %s\n",
	    deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_storage_snapshot_list(&storage_snapshots, NULL);
  deltacloud_free_storage_snapshot_list(&storage_snapshots);

  rc = deltacloud_get_storage_snapshot_by_id(&api, "snap2", &storage_snapshot);
  if (rc < 0) {
    fprintf(stderr, "Failed to get storage_snapshot by ID: %s\n",
	    deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_storage_snapshot(&storage_snapshot, NULL);
  deltacloud_free_storage_snapshot(&storage_snapshot);

  fprintf(stderr, "--------------CREATE INSTANCE---------------\n");
  rc = deltacloud_create_instance(&api, "img3", NULL, NULL, NULL, &newinstance);
  if (rc < 0) {
    fprintf(stderr, "Failed to create_instance: %s\n", deltacloud_strerror(rc));
    goto cleanup;
  }
  deltacloud_print_instance(&newinstance, NULL);
  rc = deltacloud_instance_stop(&api, &newinstance);
  if (rc < 0)
    fprintf(stderr, "Failed to stop instance: %s\n", deltacloud_strerror(rc));
  deltacloud_print_instance(&newinstance, NULL);
  rc = deltacloud_instance_start(&api, &newinstance);
  if (rc < 0)
    fprintf(stderr, "Failed to start instance: %s\n", deltacloud_strerror(rc));
  deltacloud_print_instance(&newinstance, NULL);
  rc = deltacloud_instance_reboot(&api, &newinstance);
  if (rc < 0)
    fprintf(stderr, "Failed to reboot instance: %s\n", deltacloud_strerror(rc));
  deltacloud_print_instance(&newinstance, NULL);
  rc = deltacloud_instance_destroy(&api, &newinstance);
  if (rc < 0)
    fprintf(stderr, "Failed to destroy instance: %s\n", deltacloud_strerror(rc));
  deltacloud_print_instance(&newinstance, NULL);
  deltacloud_free_instance(&newinstance);

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
