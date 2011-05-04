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

#ifndef LIBDELTACLOUD_API_H
#define LIBDELTACLOUD_API_H

#include "link.h"
#include "instance.h"
#include "realm.h"
#include "image.h"
#include "instance_state.h"
#include "storage_volume.h"
#include "storage_snapshot.h"
#include "hardware_profile.h"
#include "key.h"
#include "driver.h"
#include "loadbalancer.h"

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_api {
  char *url;
  char *user;
  char *password;
  char *driver;
  char *version;

  struct deltacloud_link *links;
};

struct deltacloud_error {
  int error_num;
  char *details;
};

struct deltacloud_create_parameter {
  char *name;
  char *value;
};

int deltacloud_initialize(struct deltacloud_api *api, char *url, char *user,
			  char *password);

#define deltacloud_supports_instances(api) deltacloud_has_link(api, "instances")
int deltacloud_get_instances(struct deltacloud_api *api,
			     struct deltacloud_instance **instances);
int deltacloud_get_instance_by_id(struct deltacloud_api *api, const char *id,
				  struct deltacloud_instance *instance);

#define deltacloud_supports_realms(api) deltacloud_has_link(api, "realms")
int deltacloud_get_realms(struct deltacloud_api *api,
			  struct deltacloud_realm **realms);
int deltacloud_get_realm_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_realm *realm);

#define deltacloud_supports_images(api) deltacloud_has_link(api, "images")
int deltacloud_get_images(struct deltacloud_api *api,
			  struct deltacloud_image **images);
int deltacloud_get_image_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_image *image);
int deltacloud_create_image(struct deltacloud_api *api, const char *instance_id,
			    struct deltacloud_create_parameter *params,
			    int params_length);

#define deltacloud_supports_instance_states(api) deltacloud_has_link(api, "instance_states")
int deltacloud_get_instance_states(struct deltacloud_api *api,
				   struct deltacloud_instance_state **instance_states);
int deltacloud_get_instance_state_by_name(struct deltacloud_api *api,
					  const char *name,
					  struct deltacloud_instance_state *instance_state);

#define deltacloud_supports_storage_volumes(api) deltacloud_has_link(api, "storage_volumes")
int deltacloud_get_storage_volumes(struct deltacloud_api *api,
				   struct deltacloud_storage_volume **storage_volumes);
int deltacloud_get_storage_volume_by_id(struct deltacloud_api *api,
					const char *id,
					struct deltacloud_storage_volume *storage_volume);

#define deltacloud_supports_storage_snapshots(api) deltacloud_has_link(api, "storage_snapshots")
int deltacloud_get_storage_snapshots(struct deltacloud_api *api,
				     struct deltacloud_storage_snapshot **storage_snapshots);
int deltacloud_get_storage_snapshot_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_storage_snapshot *storage_snapshot);

#define deltacloud_supports_hardware_profiles(api) deltacloud_has_link(api, "hardware_profiles")
int deltacloud_get_hardware_profiles(struct deltacloud_api *api,
				     struct deltacloud_hardware_profile **hardware_profiles);
int deltacloud_get_hardware_profile_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_hardware_profile *profile);

#define deltacloud_supports_keys(api) deltacloud_has_link(api, "keys")
int deltacloud_get_keys(struct deltacloud_api *api,
			struct deltacloud_key **keys);
int deltacloud_get_key_by_id(struct deltacloud_api *api, const char *id,
			     struct deltacloud_key *key);
int deltacloud_create_key(struct deltacloud_api *api, const char *name,
			  struct deltacloud_create_parameter *params,
			  int params_length);
int deltacloud_key_destroy(struct deltacloud_api *api,
			   struct deltacloud_key *key);

#define deltacloud_supports_drivers(api) deltacloud_has_link(api, "drivers")
int deltacloud_get_drivers(struct deltacloud_api *api,
			   struct deltacloud_driver **drivers);
int deltacloud_get_driver_by_id(struct deltacloud_api *api, const char *id,
				struct deltacloud_driver *driver);

#define deltacloud_supports_loadbalancers(api) deltacloud_has_link(api, "load_balancers")
int deltacloud_get_loadbalancers(struct deltacloud_api *api,
				 struct deltacloud_loadbalancer **balancers);
int deltacloud_get_loadbalancer_by_id(struct deltacloud_api *api,
				      const char *id,
				      struct deltacloud_loadbalancer *balancer);
int deltacloud_create_loadbalancer(struct deltacloud_api *api, const char *name,
				   const char *realm_id, const char *protocol,
				   int balancer_port, int instance_port,
				   struct deltacloud_create_parameter *params,
				   int params_length);
int deltacloud_loadbalancer_destroy(struct deltacloud_api *api,
				    struct deltacloud_loadbalancer *balancer);

int deltacloud_prepare_parameter(struct deltacloud_create_parameter *param,
				 const char *name, const char *value);
struct deltacloud_create_parameter *deltacloud_allocate_parameter(const char *name,
								  const char *value);
void deltacloud_free_parameter_value(struct deltacloud_create_parameter *param);
void deltacloud_free_parameter(struct deltacloud_create_parameter *param);
int deltacloud_create_instance(struct deltacloud_api *api, const char *image_id,
			       struct deltacloud_create_parameter *params,
			       int params_length, char **instance_id);
int deltacloud_instance_stop(struct deltacloud_api *api,
			     struct deltacloud_instance *instance);
int deltacloud_instance_reboot(struct deltacloud_api *api,
			       struct deltacloud_instance *instance);
int deltacloud_instance_start(struct deltacloud_api *api,
			      struct deltacloud_instance *instance);
int deltacloud_instance_destroy(struct deltacloud_api *api,
				struct deltacloud_instance *instance);

struct deltacloud_error *deltacloud_get_last_error(void);
const char *deltacloud_get_last_error_string(void);

int deltacloud_has_link(struct deltacloud_api *api, const char *name);

void deltacloud_free(struct deltacloud_api *api);

#define deltacloud_for_each(curr, list) for (curr = list; curr != NULL; curr = curr->next)

/* Error codes */
#define DELTACLOUD_UNKNOWN_ERROR -1
/* ERROR codes -2, -3, and -4 are reserved for future use */
#define DELTACLOUD_GET_URL_ERROR -5
#define DELTACLOUD_POST_URL_ERROR -6
#define DELTACLOUD_XML_ERROR -7
#define DELTACLOUD_URL_DOES_NOT_EXIST_ERROR -8
#define DELTACLOUD_OOM_ERROR -9
#define DELTACLOUD_INVALID_ARGUMENT_ERROR -10
#define DELTACLOUD_NAME_NOT_FOUND_ERROR -11
#define DELTACLOUD_DELETE_URL_ERROR -12

#ifdef __cplusplus
}
#endif

#endif
