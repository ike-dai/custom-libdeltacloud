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

#ifndef LIBDELTACLOUD_STORAGE_VOLUME_H
#define LIBDELTACLOUD_STORAGE_VOLUME_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing the storage volume capacity.
 */
struct deltacloud_storage_volume_capacity {
  char *unit; /**< The units the capacity is specified in (MB, GB, etc) */
  char *size; /**< The size of the storage volume */
};

/**
 * A structure representing a single storage volume mount.  This structure
 * is populated if a storage volume is mounted to an instance.
 */
struct deltacloud_storage_volume_mount {
  char *instance_href; /**< The full URL to the instance this storage volume is attached to */
  char *instance_id; /**< The ID of the instance this storage volume is attached to */
  char *device_name; /**< The device this storage volume is attached as */
};

/**
 * A structure representing a single storage volume.
 */
struct deltacloud_storage_volume {
  char *href; /**< The full URL to this storage volume */
  char *id; /**< The ID of this storage volume */
  char *created; /**< The time this storage volume was created */
  char *state; /**< The state of this storage volume */
  struct deltacloud_storage_volume_capacity capacity; /**< The capacity of this storage volume */
  char *device; /**< The device this storage volume is attached as */
  char *realm_id; /**< The ID of the realm this storage volume is in */
  struct deltacloud_storage_volume_mount mount; /**< The device this storage volume is mounted as */

  struct deltacloud_storage_volume *next;
};

#define deltacloud_supports_storage_volumes(api) deltacloud_has_link(api, "storage_volumes")
int deltacloud_get_storage_volumes(struct deltacloud_api *api,
				   struct deltacloud_storage_volume **storage_volumes);
int deltacloud_get_storage_volume_by_id(struct deltacloud_api *api,
					const char *id,
					struct deltacloud_storage_volume *storage_volume);
int deltacloud_create_storage_volume(struct deltacloud_api *api,
				     struct deltacloud_create_parameter *params,
				     int params_length);
int deltacloud_storage_volume_destroy(struct deltacloud_api *api,
				      struct deltacloud_storage_volume *storage_volume);
int deltacloud_storage_volume_attach(struct deltacloud_api *api,
				     struct deltacloud_storage_volume *storage_volume,
				     const char *instance_id,
				     const char *device,
				     struct deltacloud_create_parameter *params,
				     int params_length);
int deltacloud_storage_volume_detach(struct deltacloud_api *api,
				     struct deltacloud_storage_volume *storage_volume,
				     struct deltacloud_create_parameter *params,
				     int params_length);
void deltacloud_free_storage_volume(struct deltacloud_storage_volume *storage_volume);
void deltacloud_free_storage_volume_list(struct deltacloud_storage_volume **storage_volumes);

#ifdef __cplusplus
}
#endif

#endif
