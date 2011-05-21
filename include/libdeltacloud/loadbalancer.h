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

#ifndef LIBDELTACLOUD_LOADBALANCER_H
#define LIBDELTACLOUD_LOADBALANCER_H

#include "action.h"
#include "address.h"
#include "link.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing a single listener for this load balancer.  A
 * listener consists of a protocol (TCP, HTTP, etc), a port that the load
 * balancer listens on, and a port that the load balancer balances to.
 */
struct deltacloud_loadbalancer_listener {
  char *protocol; /**< The protocol to listen for */
  char *load_balancer_port; /**< The port the load balancer listens on */
  char *instance_port; /**< The port on the instance the load balancer balances to */

  struct deltacloud_loadbalancer_listener *next;
};

/**
 * A structure representing a single instance connected to a load balancer.
 */
struct deltacloud_loadbalancer_instance {
  char *href; /**< The full URL to the instance */
  char *id; /**< The ID of the instance */
  struct deltacloud_link *links; /**< A list of links */

  struct deltacloud_loadbalancer_instance *next;
};

/**
 * A structure representing a single load balancer.
 */
struct deltacloud_loadbalancer {
  char *href; /**< The full URL to the load balancer */
  char *id; /**< The ID of the load balancer */
  struct deltacloud_action *actions; /**< A list of actions that can be taken on this load balancer */
  struct deltacloud_address *public_addresses; /**< A list of public addresses assigned to this load balancer */
  char *created_at; /**< The time at which this load balancer was created */
  char *realm_href; /**< The full URL to the realm this load balancer is in */
  char *realm_id; /**< The ID of the realm this load balancer is in */
  struct deltacloud_loadbalancer_listener *listeners; /**< A list of protocols/ports that this load balancer is listening on */
  struct deltacloud_loadbalancer_instance *instances; /**< A list of instances attached to this load balancer */

  struct deltacloud_loadbalancer *next;
};

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
int deltacloud_loadbalancer_register(struct deltacloud_api *api,
				     struct deltacloud_loadbalancer *balancer,
				     const char *instance_id,
				     struct deltacloud_create_parameter *params,
				     int params_length);
int deltacloud_loadbalancer_unregister(struct deltacloud_api *api,
				       struct deltacloud_loadbalancer *balancer,
				       const char *instance_id,
				       struct deltacloud_create_parameter *params,
				       int params_length);
int deltacloud_loadbalancer_destroy(struct deltacloud_api *api,
				    struct deltacloud_loadbalancer *balancer);
void deltacloud_free_loadbalancer(struct deltacloud_loadbalancer *lb);
void deltacloud_free_loadbalancer_list(struct deltacloud_loadbalancer **lbs);

#ifdef __cplusplus
}
#endif

#endif
