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

#ifndef LIBDELTACLOUD_HARDWARE_PROFILE_H
#define LIBDELTACLOUD_HARDWARE_PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing a single value for a deltacloud hardware profile
 * enum.
 */
struct deltacloud_property_enum {
  char *value; /**< The value for this enum */

  struct deltacloud_property_enum *next;
};

/**
 * A structure representing a single deltacloud hardware profile property range.
 * A range represents a range of values for this property.
 */
struct deltacloud_property_range {
  char *first; /**< The start of this range */
  char *last; /**< The end of this range */

  struct deltacloud_property_range *next;
};

/**
 * A structure representing a single deltacloud hardware profile param.
 */
struct deltacloud_property_param {
  char *href; /**< The full URL to this parameter */
  char *method; /**< The type of HTTP request (GET, POST, etc) to use */
  char *name; /**< The name of this param */
  char *operation; /**< The operation of this param */

  struct deltacloud_property_param *next;
};

/**
 * A structure representing a single deltacloud hardware profile property.
 * A property can be a deltacloud_property_param, a deltacloud_property_range,
 * or a deltacloud_property_enum.
 */
struct deltacloud_property {
  char *kind; /**< The type of property this is (param, range, or enum) */
  char *name; /**< The name of this property */
  char *unit; /**< The units this property is specified in (MB, GB, etc) */
  char *value; /**< The value for this property */

  struct deltacloud_property_param *params; /**< A list of params associated with this property */
  struct deltacloud_property_enum *enums; /**< A list of enums associated with this property */
  struct deltacloud_property_range *ranges; /**< A list of ranges associated with this property */

  struct deltacloud_property *next;
};

/**
 * A structure representing a single deltacloud hardware profile.
 */
struct deltacloud_hardware_profile {
  char *href; /**< The full URL to this hardware profile */
  char *id; /**< The ID of this hardware profile */
  char *name; /**< The name of this hardware profile */

  struct deltacloud_property *properties; /**< A list of deltacloud_property structures */

  struct deltacloud_hardware_profile *next;
};

#define deltacloud_supports_hardware_profiles(api) deltacloud_has_link(api, "hardware_profiles")
int deltacloud_get_hardware_profiles(struct deltacloud_api *api,
				     struct deltacloud_hardware_profile **hardware_profiles);
int deltacloud_get_hardware_profile_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_hardware_profile *profile);
void deltacloud_free_hardware_profile(struct deltacloud_hardware_profile *profile);
void deltacloud_free_hardware_profile_list(struct deltacloud_hardware_profile **profiles);

#ifdef __cplusplus
}
#endif

#endif
