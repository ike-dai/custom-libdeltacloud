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

#ifndef INSTANCES_H
#define INSTANCES_H

#include "hardware_profile.h"

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_action {
  char *rel;
  char *href;
  char *method;

  struct deltacloud_action *next;
};

struct deltacloud_address {
  char *address;

  struct deltacloud_address *next;
};

struct deltacloud_instance {
  char *href;
  char *id;
  char *name;
  char *owner_id;
  char *image_id;
  char *image_href;
  char *realm_id;
  char *realm_href;
  char *key_name;
  char *state;
  struct deltacloud_hardware_profile hwp;
  struct deltacloud_action *actions;
  struct deltacloud_address *public_addresses;
  struct deltacloud_address *private_addresses;

  struct deltacloud_instance *next;
};

void free_address(struct deltacloud_address *addr);
int add_to_address_list(struct deltacloud_address **addreses,
			struct deltacloud_address *address);
void free_address_list(struct deltacloud_address **addresses);

void free_action(struct deltacloud_action *action);
int add_to_action_list(struct deltacloud_action **actions,
		       struct deltacloud_action *action);
void free_action_list(struct deltacloud_action **actions);

int add_to_instance_list(struct deltacloud_instance **instances,
			 struct deltacloud_instance *instance);
int copy_instance(struct deltacloud_instance *dst,
		  struct deltacloud_instance *src);
void deltacloud_free_instance(struct deltacloud_instance *instance);
void deltacloud_free_instance_list(struct deltacloud_instance **instances);

#ifdef __cplusplus
}
#endif

#endif
