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

#ifndef REALM_H
#define REALM_H

struct realm {
  char *href;
  char *id;
  char *name;
  char *limit;
  char *state;

  struct realm *next;
};

int add_to_realm_list(struct realm **realms, char *href, char *id, char *name,
		      char *state, char *limit);
void print_realm(struct realm *realm, FILE *stream);
void print_realm_list(struct realm **realms, FILE *stream);
int copy_realm(struct realm *dst, struct realm *src);
void free_realm(struct realm *realm);
void free_realm_list(struct realm **realms);

#endif
