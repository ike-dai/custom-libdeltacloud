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

#ifndef LIBDELTACLOUD_STORAGE_SNAPSHOT_H
#define LIBDELTACLOUD_STORAGE_SNAPSHOT_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_storage_snapshot {
  char *href;
  char *id;
  char *created;
  char *state;
  char *storage_volume_href;
  char *storage_volume_id;

  struct deltacloud_storage_snapshot *next;
};

#define deltacloud_supports_storage_snapshots(api) deltacloud_has_link(api, "storage_snapshots")
int deltacloud_get_storage_snapshots(struct deltacloud_api *api,
				     struct deltacloud_storage_snapshot **storage_snapshots);
int deltacloud_get_storage_snapshot_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_storage_snapshot *storage_snapshot);
int deltacloud_create_storage_snapshot(struct deltacloud_api *api,
				       const char *volume_id,
				       struct deltacloud_create_parameter *params,
				       int params_length);
int deltacloud_storage_snapshot_destroy(struct deltacloud_api *api,
					struct deltacloud_storage_snapshot *storage_snapshot);
void deltacloud_free_storage_snapshot(struct deltacloud_storage_snapshot *storage_snapshot);
void deltacloud_free_storage_snapshot_list(struct deltacloud_storage_snapshot **storage_snapshots);

#ifdef __cplusplus
}
#endif

#endif
