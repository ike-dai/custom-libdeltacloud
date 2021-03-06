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

#ifndef LIBDELTACLOUD_INSTANCE_H
#define LIBDELTACLOUD_INSTANCE_H

#include "hardware_profile.h"
#include "action.h"
#include "address.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing an instance authentication method.  Each instance
 * only has a single authentication method, which may either be via key or
 * via username/password (not both).
 */
struct deltacloud_instance_auth {
  char *type; /**< The type of authentication method in use */

  char *keyname; /**< The name of the key to connect to the instance */

  char *username; /**< The username to use to connect to the instance */
  char *password; /**< The password to use to connect to the instance */
};

/**
 * A structure representing a single deltacloud instance.
 */
struct deltacloud_instance {
  char *href; /**< The full URL to this instance */
  char *id; /**< The ID of this instance */
  char *name; /**< The name of this instance */
  char *owner_id; /**< The owner ID of this instance */
  char *image_id; /**< The ID of this image this instance was launched from */
  char *image_href; /**< The full URL to the image */
  char *realm_id; /**< The ID of the realm this instance is in */
  char *realm_href; /**< The full URL to the realm this instance is in */
  char *state; /**< The current state of this instance (RUNNING, STOPPED, etc) */
  char *launch_time; /**< The time that this instance was launched */
  struct deltacloud_hardware_profile hwp; /**< The hardware profile this instance was launched with */
  struct deltacloud_action *actions; /**< A list of actions that can be taken on this instance */
  struct deltacloud_address *public_addresses; /**< A list of the public addresses assigned to this instance */
  struct deltacloud_address *private_addresses; /**< A list of the private addresses assigned to this instance */
  struct deltacloud_instance_auth auth; /**< The authentication method used to connect to this instance */

  struct deltacloud_instance *next;
};

#define deltacloud_supports_instances(api) deltacloud_has_link(api, "instances")
int deltacloud_get_instances(struct deltacloud_api *api,
			     struct deltacloud_instance **instances);
int deltacloud_get_instance_by_id(struct deltacloud_api *api, const char *id,
				  struct deltacloud_instance *instance);
int deltacloud_get_instance_by_name(struct deltacloud_api *api,
				    const char *name,
				    struct deltacloud_instance *instance);
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
void deltacloud_free_instance(struct deltacloud_instance *instance);
void deltacloud_free_instance_list(struct deltacloud_instance **instances);

#ifdef __cplusplus
}
#endif

#endif
