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

struct flavor {
  char *href;
  char *id;
  char *memory;
  char *storage;
  char *architecture;

  struct flavor *next;
};

int copy_flavor(struct flavor *dst, struct flavor *src);
int add_to_flavor_list(struct flavor **flavors, const char *href,
		       const char *id, const char *memory, const char *storage,
		       const char *architecture);
void print_flavor(struct flavor *flavor, FILE *stream);
void print_flavor_list(struct flavor **flavors, FILE *stream);
void free_flavor(struct flavor *flavor);
void free_flavor_list(struct flavor **flavors);

#endif
