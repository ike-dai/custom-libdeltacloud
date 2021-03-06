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

#ifndef LIBDELTACLOUD_REALM_H
#define LIBDELTACLOUD_REALM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing a single realm.
 */
struct deltacloud_realm {
  char *href; /**< The full URL to this realm */
  char *id; /**< The ID of this realm */
  char *name; /**< The name of this realm */
  char *limit; /**< The limit of this realm */
  char *state; /**< The state of this realm */

  struct deltacloud_realm *next;
};

#define deltacloud_supports_realms(api) deltacloud_has_link(api, "realms")
int deltacloud_get_realms(struct deltacloud_api *api,
			  struct deltacloud_realm **realms);
int deltacloud_get_realm_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_realm *realm);
void deltacloud_free_realm(struct deltacloud_realm *realm);
void deltacloud_free_realm_list(struct deltacloud_realm **realms);

#ifdef __cplusplus
}
#endif

#endif
