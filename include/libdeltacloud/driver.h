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

#ifndef LIBDELTACLOUD_DRIVER_H
#define LIBDELTACLOUD_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_driver_provider {
  char *id;

  struct deltacloud_driver_provider *next;
};

struct deltacloud_driver {
  char *href;
  char *id;
  char *name;
  struct deltacloud_driver_provider *providers;

  struct deltacloud_driver *next;
};

#define deltacloud_supports_drivers(api) deltacloud_has_link(api, "drivers")
int deltacloud_get_drivers(struct deltacloud_api *api,
			   struct deltacloud_driver **drivers);
int deltacloud_get_driver_by_id(struct deltacloud_api *api, const char *id,
				struct deltacloud_driver *driver);

void deltacloud_free_driver(struct deltacloud_driver *driver);
void deltacloud_free_driver_list(struct deltacloud_driver **drivers);

#ifdef __cplusplus
}
#endif

#endif
