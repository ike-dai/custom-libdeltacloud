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
  char *image_href;
  char *realm_href;
  char *key_name;
  char *state;
  struct deltacloud_hardware_profile hwp;
  struct deltacloud_action *actions;
  struct deltacloud_address *public_addresses;
  struct deltacloud_address *private_addresses;

  struct deltacloud_instance *next;
};

int add_to_address_list(struct deltacloud_address **addreses,
			const char *address);
void print_address_list(struct deltacloud_address **addresses, FILE *stream);
void free_address_list(struct deltacloud_address **addresses);

int add_to_action_list(struct deltacloud_action **actions, const char *rel,
		       const char *href);
struct deltacloud_action *find_by_rel_in_action_list(struct deltacloud_action **actions,
						     const char *rel);
void print_action_list(struct deltacloud_action **actions, FILE *stream);
void free_action_list(struct deltacloud_action **actions);

int add_to_instance_list(struct deltacloud_instance **instances,
			 const char *href, const char *id, const char *name,
			 const char *owner_id, const char *image_href,
			 const char *realm_href, const char *state,
			 struct deltacloud_hardware_profile *hwp,
			 struct deltacloud_action *actions,
			 struct deltacloud_address *public_addresses,
			 struct deltacloud_address *private_addresses);
int copy_instance(struct deltacloud_instance *dst,
		  struct deltacloud_instance *src);
struct deltacloud_instance *find_by_name_in_instance_list(struct deltacloud_instance **instances,
							  const char *name);
void deltacloud_print_instance(struct deltacloud_instance *instance,
			       FILE *stream);
void deltacloud_print_instance_list(struct deltacloud_instance **instances,
				    FILE *stream);
void deltacloud_free_instance(struct deltacloud_instance *instance);
void deltacloud_free_instance_list(struct deltacloud_instance **instances);

#ifdef __cplusplus
}
#endif

#endif
