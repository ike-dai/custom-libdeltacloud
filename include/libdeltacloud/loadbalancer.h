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

struct deltacloud_loadbalancer_listener {
  char *protocol;
  char *load_balancer_port;
  char *instance_port;

  struct deltacloud_loadbalancer_listener *next;
};

struct deltacloud_loadbalancer_instance {
  char *href;
  char *id;
  struct deltacloud_link *links;

  struct deltacloud_loadbalancer_instance *next;
};

struct deltacloud_loadbalancer {
  char *href;
  char *id;
  struct deltacloud_action *actions;
  struct deltacloud_address *public_addresses;
  char *created_at;
  char *realm_href;
  char *realm_id;
  struct deltacloud_loadbalancer_listener *listeners;
  struct deltacloud_loadbalancer_instance *instances;

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
int deltacloud_loadbalancer_destroy(struct deltacloud_api *api,
				    struct deltacloud_loadbalancer *balancer);

void deltacloud_free_loadbalancer(struct deltacloud_loadbalancer *lb);
void deltacloud_free_loadbalancer_list(struct deltacloud_loadbalancer **lbs);

#ifdef __cplusplus
}
#endif

#endif
