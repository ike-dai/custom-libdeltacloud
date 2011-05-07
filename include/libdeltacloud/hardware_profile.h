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

#include <libxml/parser.h>
#include <libxml/xpath.h>

struct deltacloud_property_enum {
  char *value;

  struct deltacloud_property_enum *next;
};

struct deltacloud_property_range {
  char *first;
  char *last;

  struct deltacloud_property_range *next;
};

struct deltacloud_property_param {
  char *href;
  char *method;
  char *name;
  char *operation;

  struct deltacloud_property_param *next;
};

struct deltacloud_property {
  char *kind;
  char *name;
  char *unit;
  char *value;

  struct deltacloud_property_param *params;

  /* these next two are mutually exclusive */
  struct deltacloud_property_enum *enums;
  struct deltacloud_property_range *ranges;

  struct deltacloud_property *next;
};

struct deltacloud_hardware_profile {
  char *href;
  char *id;
  char *name;

  struct deltacloud_property *properties;

  struct deltacloud_hardware_profile *next;
};

/* these are exported for internal use only */
int parse_hardware_profile_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			       void **data);
int copy_hardware_profile(struct deltacloud_hardware_profile *dst,
			  struct deltacloud_hardware_profile *src);

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
