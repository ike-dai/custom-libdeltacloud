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

static void print_storage_volume(struct deltacloud_storage_volume *volume)
{
  fprintf(stderr, "Storage Volume: %s\n", volume->id);
  fprintf(stderr, "\tHref: %s\n", volume->href);
  fprintf(stderr, "\tCreated: %s, State: %s\n", volume->created, volume->state);
  fprintf(stderr, "\tCapacity: Unit: %s, Size: %s\n", volume->capacity.unit,
	  volume->capacity.size);
  fprintf(stderr, "\tDevice: %s\n", volume->device);
  fprintf(stderr, "\tInstance Href: %s\n", volume->instance_href);
  fprintf(stderr, "\tRealm ID: %s\n", volume->realm_id);
  fprintf(stderr, "\tMount: %s\n", volume->mount.instance_id);
  fprintf(stderr, "\t\tInstance Href: %s\n", volume->mount.instance_href);
  fprintf(stderr, "\t\tDevice Name: %s\n", volume->mount.device_name);
}

static void print_storage_volume_list(struct deltacloud_storage_volume *volumes)
{
  struct deltacloud_storage_volume *volume;

  deltacloud_for_each(volume, volumes)
    print_storage_volume(volume);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_storage_volume *storage_volumes;
  struct deltacloud_storage_volume storage_volume;
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

  if (deltacloud_supports_storage_volumes(&api)) {
    if (deltacloud_get_storage_volumes(&api, &storage_volumes) < 0) {
      fprintf(stderr, "Failed to get_storage_volumes: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_storage_volume_list(storage_volumes);

    if (storage_volumes != NULL) {
      /* here we use the first storage volume from the list above */
      if (deltacloud_get_storage_volume_by_id(&api, storage_volumes->id,
					      &storage_volume) < 0) {
	fprintf(stderr, "Failed to get storage volume by ID: %s\n",
		deltacloud_get_last_error_string());
	deltacloud_free_storage_volume_list(&storage_volumes);
	goto cleanup;
      }
      print_storage_volume(&storage_volume);
      deltacloud_free_storage_volume(&storage_volume);
    }

    deltacloud_free_storage_volume_list(&storage_volumes);
  }
  else
    fprintf(stderr, "Storage Volumes are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
