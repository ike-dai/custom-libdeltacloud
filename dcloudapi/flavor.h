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

#ifndef FLAVOR_H
#define FLAVOR_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_flavor {
  char *href;
  char *id;
  char *memory;
  char *storage;
  char *architecture;

  struct deltacloud_flavor *next;
};

int copy_flavor(struct deltacloud_flavor *dst, struct deltacloud_flavor *src);
int add_to_flavor_list(struct deltacloud_flavor **flavors, const char *href,
		       const char *id, const char *memory, const char *storage,
		       const char *architecture);
void deltacloud_print_flavor(struct deltacloud_flavor *flavor, FILE *stream);
void deltacloud_print_flavor_list(struct deltacloud_flavor **flavors,
				  FILE *stream);
void deltacloud_free_flavor(struct deltacloud_flavor *flavor);
void deltacloud_free_flavor_list(struct deltacloud_flavor **flavors);

#ifdef __cplusplus
}
#endif

#endif
