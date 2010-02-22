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

#ifdef __cplusplus
extern "C" {
#endif

struct action {
  char *rel;
  char *href;

  struct action *next;
};

struct address {
  char *address;

  struct address *next;
};

struct instance {
  char *id;
  char *name;
  char *owner_id;
  char *image_href;
  char *flavor_href;
  char *realm_href;
  char *state;
  struct action *actions;
  struct address *public_addresses;
  struct address *private_addresses;

  struct instance *next;
};

int add_to_address_list(struct address **addreses, const char *address);
void print_address_list(struct address **addresses, FILE *stream);
void free_address_list(struct address **addresses);

int add_to_action_list(struct action **actions, const char *rel,
		       const char *href);
struct action *find_by_rel_in_action_list(struct action **actions,
					  const char *rel);
void print_action_list(struct action **actions, FILE *stream);
void free_action_list(struct action **actions);

int add_to_instance_list(struct instance **instances, const char *id,
			 const char *name, const char *owner_id,
			 const char *image_href, const char *flavor_href,
			 const char *realm_href, const char *state,
			 struct action *actions,
			 struct address *public_addresses,
			 struct address *private_addresses);
int copy_instance(struct instance *dst, struct instance *src);
void print_instance(struct instance *instance, FILE *stream);
void print_instance_list(struct instance **instances, FILE *stream);
void free_instance(struct instance *instance);
void free_instance_list(struct instance **instances);

#ifdef __cplusplus
}
#endif

#endif
