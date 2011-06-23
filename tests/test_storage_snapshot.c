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

static void print_storage_snapshot(struct deltacloud_storage_snapshot *snapshot)
{
  fprintf(stderr, "Snapshot: %s\n", snapshot->id);
  fprintf(stderr, "\tHref: %s\n", snapshot->href);
  fprintf(stderr, "\tCreated: %s, State: %s\n", snapshot->created,
	  snapshot->state);
  fprintf(stderr, "\tStorage Volume Href: %s\n", snapshot->storage_volume_href);
  fprintf(stderr, "\tStorage Volume ID: %s\n", snapshot->storage_volume_id);
}

static void print_storage_snapshot_list(struct deltacloud_storage_snapshot *snapshots)
{
  struct deltacloud_storage_snapshot *snapshot;

  deltacloud_for_each(snapshot, snapshots)
    print_storage_snapshot(snapshot);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_api zeroapi;
  struct deltacloud_storage_snapshot *storage_snapshots;
  struct deltacloud_storage_snapshot storage_snapshot;
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

  memset(&zeroapi, 0, sizeof(struct deltacloud_api));

  /* test out deltacloud_supports_storage_snapshots */
  if (deltacloud_supports_storage_snapshots(NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_storage_snapshots to fail with NULL api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_storage_snapshots(&zeroapi) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_storage_snapshots to fail with uninitialized api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_storage_snapshots(&api)) {

    /* test out deltacloud_get_storage_snapshots */
    if (deltacloud_get_storage_snapshots(NULL, &storage_snapshots) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_storage_snapshots to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_storage_snapshots(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_storage_snapshots to fail with NULL storage_snapshots, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_storage_snapshots(&zeroapi, &storage_snapshots) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_storage_snapshots to fail with unintialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_storage_snapshots(&api, &storage_snapshots) < 0) {
      fprintf(stderr, "Failed to get_storage_snapshots: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_storage_snapshot_list(storage_snapshots);

    if (storage_snapshots != NULL) {

      /* test out deltacloud_get_storage_snapshot_by_id */
      if (deltacloud_get_storage_snapshot_by_id(NULL, storage_snapshots->id,
						&storage_snapshot) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_storage_snapshot_by_id to fail with NULL api, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_storage_snapshot_by_id(&api, NULL,
						&storage_snapshot) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_storage_snapshot_by_id to fail with NULL id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_storage_snapshot_by_id(&api, storage_snapshots->id,
						NULL) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_storage_snapshot_by_id to fail with NULL storage_snapshot, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_storage_snapshot_by_id(&api, "bogus_id",
						&storage_snapshot) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_storage_snapshot_by_id to fail with bogus id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_storage_snapshot_by_id(&zeroapi, storage_snapshots->id,
						&storage_snapshot) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_storage_snapshot_by_id to fail with unintialized api, but succeeded\n");
	goto cleanup;
      }

      /* here we use the first storage snapshot from the list above */
      if (deltacloud_get_storage_snapshot_by_id(&api, storage_snapshots->id,
						&storage_snapshot) < 0) {
	fprintf(stderr, "Failed to get storage_snapshot by ID: %s\n",
		deltacloud_get_last_error_string());
	deltacloud_free_storage_snapshot_list(&storage_snapshots);
	goto cleanup;
      }
      print_storage_snapshot(&storage_snapshot);
      deltacloud_free_storage_snapshot(&storage_snapshot);
    }

    deltacloud_free_storage_snapshot_list(&storage_snapshots);
  }
  else
    fprintf(stderr, "Storage Snapshots are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
