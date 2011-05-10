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

#ifndef LIBDELTACLOUD_KEY_H
#define LIBDELTACLOUD_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_key {
  char *href;
  char *id;
  char *type;
  char *state;
  char *fingerprint;

  struct deltacloud_key *next;
};

#define deltacloud_supports_keys(api) deltacloud_has_link(api, "keys")
int deltacloud_get_keys(struct deltacloud_api *api,
			struct deltacloud_key **keys);
int deltacloud_get_key_by_id(struct deltacloud_api *api, const char *id,
			     struct deltacloud_key *key);
int deltacloud_create_key(struct deltacloud_api *api, const char *name,
			  struct deltacloud_create_parameter *params,
			  int params_length);
int deltacloud_key_destroy(struct deltacloud_api *api,
			   struct deltacloud_key *key);
void deltacloud_free_key(struct deltacloud_key *key);
void deltacloud_free_key_list(struct deltacloud_key **keys);

#ifdef __cplusplus
}
#endif

#endif
