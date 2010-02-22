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
  struct instance *instances;
  struct image *images;
  struct flavor *flavors;
  struct realm *realms;
  struct instance_state *instance_states;
  struct storage_volume *storage_volumes;
  struct storage_snapshot *storage_snapshots;
  struct flavor flavor;
  struct instance_state instance_state;
  struct realm realm;
  struct image image;
  struct instance instance;
  struct storage_volume storage_volume;
  struct storage_snapshot storage_snapshot;
  struct instance *newinstance;
  char *fullurl;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  fprintf(stderr, "--------------LINKS--------------------------\n");
  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API\n");
    return 2;
  }
  print_link_list(&api.links, NULL);

  fprintf(stderr, "--------------IMAGES-------------------------\n");
  if (deltacloud_get_images(&api, &images) < 0) {
    fprintf(stderr, "Failed to get_images\n");
    goto cleanup;
  }
  print_image_list(&images, NULL);
  free_image_list(&images);

  if (deltacloud_get_image_by_id(&api, "img1", &image) < 0) {
    fprintf(stderr, "Failed to get image by id\n");
    goto cleanup;
  }
  print_image(&image, NULL);
  free_image(&image);

  fprintf(stderr, "--------------FLAVORS------------------------\n");
  if (deltacloud_get_flavors(&api, &flavors) < 0) {
    fprintf(stderr, "Failed to get_flavors\n");
    goto cleanup;
  }
  print_flavor_list(&flavors, NULL);
  free_flavor_list(&flavors);

  if (deltacloud_get_flavor_by_id(&api, "m1-small", &flavor) < 0) {
    fprintf(stderr, "Failed to get 'm1-small' flavor\n");
    goto cleanup;
  }
  print_flavor(&flavor, NULL);
  free_flavor(&flavor);

  if (asprintf(&fullurl, "%s/flavors/c1-medium", api.url) < 0) {
    fprintf(stderr, "Failed to allocate fullurl\n");
    goto cleanup;
  }
  if (deltacloud_get_flavor_by_uri(&api, fullurl, &flavor) < 0) {
    fprintf(stderr, "Failed to get 'c1-medium' flavor\n");
    MY_FREE(fullurl);
    goto cleanup;
  }
  print_flavor(&flavor, NULL);
  free_flavor(&flavor);
  MY_FREE(fullurl);

  fprintf(stderr, "--------------REALMS-------------------------\n");
  if (deltacloud_get_realms(&api, &realms) < 0) {
    fprintf(stderr, "Failed to get_realms\n");
    goto cleanup;
  }
  print_realm_list(&realms, NULL);
  free_realm_list(&realms);

  if (deltacloud_get_realm_by_id(&api, "us", &realm) < 0) {
    fprintf(stderr, "Failed to get realm by id\n");
    goto cleanup;
  }
  print_realm(&realm, NULL);
  free_realm(&realm);

  fprintf(stderr, "--------------INSTANCE STATES----------------\n");
  if (deltacloud_get_instance_states(&api, &instance_states) < 0) {
    fprintf(stderr, "Failed to get_instance_states\n");
    goto cleanup;
  }
  print_instance_state_list(&instance_states, NULL);
  free_instance_state_list(&instance_states);

  if (deltacloud_get_instance_state_by_name(&api, "start",
					    &instance_state) < 0) {
    fprintf(stderr, "Failed to get instance_state\n");
    goto cleanup;
  }
  print_instance_state(&instance_state, NULL);
  free_instance_state(&instance_state);

  fprintf(stderr, "--------------INSTANCES---------------------\n");
  if (deltacloud_get_instances(&api, &instances) < 0) {
    fprintf(stderr, "Failed to get_instances\n");
    goto cleanup;
  }
  print_instance_list(&instances, NULL);
  free_instance_list(&instances);

  if (deltacloud_get_instance_by_id(&api, "inst1", &instance) < 0) {
    fprintf(stderr, "Failed to get instance by id\n");
    goto cleanup;
  }
  print_instance(&instance, NULL);
  free_instance(&instance);

  fprintf(stderr, "--------------STORAGE VOLUMES---------------\n");
  if (deltacloud_get_storage_volumes(&api, &storage_volumes) < 0) {
    fprintf(stderr, "Failed to get_storage_volumes\n");
    goto cleanup;
  }
  print_storage_volume_list(&storage_volumes, NULL);
  free_storage_volume_list(&storage_volumes);

  if (deltacloud_get_storage_volume_by_id(&api, "vol3", &storage_volume) < 0) {
    fprintf(stderr, "Failed to get storage volume by ID\n");
    goto cleanup;
  }
  print_storage_volume(&storage_volume, NULL);
  free_storage_volume(&storage_volume);

  fprintf(stderr, "--------------STORAGE SNAPSHOTS-------------\n");
  if (deltacloud_get_storage_snapshots(&api, &storage_snapshots) < 0) {
    fprintf(stderr, "Failed to get_storage_snapshots\n");
    goto cleanup;
  }
  print_storage_snapshot_list(&storage_snapshots, NULL);
  free_storage_snapshot_list(&storage_snapshots);

  if (deltacloud_get_storage_snapshot_by_id(&api, "snap2",
					    &storage_snapshot) < 0) {
    fprintf(stderr, "Failed to get storage_snapshot by ID\n");
    goto cleanup;
  }
  print_storage_snapshot(&storage_snapshot, NULL);
  free_storage_snapshot(&storage_snapshot);

  fprintf(stderr, "--------------CREATE INSTANCE---------------\n");
  newinstance = deltacloud_create_instance(&api, "img3", NULL, NULL, NULL);
  if (newinstance == NULL) {
    fprintf(stderr, "Failed to create_instance\n");
    goto cleanup;
  }
  print_instance(newinstance, NULL);
  if (deltacloud_instance_stop(&api, newinstance) < 0)
    fprintf(stderr, "Failed to stop instance\n");
  print_instance(newinstance, NULL);
  if (deltacloud_instance_start(&api, newinstance) < 0)
    fprintf(stderr, "Failed to start instance\n");
  print_instance(newinstance, NULL);
  if (deltacloud_instance_reboot(&api, newinstance) < 0)
    fprintf(stderr, "Failed to reboot instance\n");
  print_instance(newinstance, NULL);
  free_instance(newinstance);
  MY_FREE(newinstance);

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
