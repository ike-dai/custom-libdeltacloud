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

#ifndef ADDRESS_H
#define ADDRESS_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_address {
  char *address;

  struct deltacloud_address *next;
};

void free_address(struct deltacloud_address *addr);
int add_to_address_list(struct deltacloud_address **addreses,
			struct deltacloud_address *address);
int copy_address_list(struct deltacloud_address **dst,
		      struct deltacloud_address **src);
void free_address_list(struct deltacloud_address **addresses);

#ifdef __cplusplus
}
#endif

#endif
