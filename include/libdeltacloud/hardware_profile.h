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

#ifndef LIBDELTACLOUD_HARDWARE_PROFILE_H
#define LIBDELTACLOUD_HARDWARE_PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif

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

int add_to_range_list(struct deltacloud_property_range **ranges,
		      struct deltacloud_property_range *range);
void free_range(struct deltacloud_property_range *onerange);
void free_range_list(struct deltacloud_property_range **ranges);

int add_to_enum_list(struct deltacloud_property_enum **enums,
		     struct deltacloud_property_enum *inenum);
void free_enum(struct deltacloud_property_enum *oneenum);
void free_enum_list(struct deltacloud_property_enum **enums);

int add_to_param_list(struct deltacloud_property_param **params,
		      struct deltacloud_property_param *param);
void free_param(struct deltacloud_property_param *param);
void free_param_list(struct deltacloud_property_param **params);

int add_to_property_list(struct deltacloud_property **props,
			 struct deltacloud_property *prop);
void free_prop(struct deltacloud_property *prop);
void free_property_list(struct deltacloud_property **props);

void deltacloud_free_hardware_profile(struct deltacloud_hardware_profile *profile);
int add_to_hardware_profile_list(struct deltacloud_hardware_profile **profiles,
				 struct deltacloud_hardware_profile *profile);
int copy_hardware_profile(struct deltacloud_hardware_profile *dst,
			  struct deltacloud_hardware_profile *src);
void deltacloud_free_hardware_profile_list(struct deltacloud_hardware_profile **profiles);

#ifdef __cplusplus
}
#endif

#endif
