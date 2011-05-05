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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "hardware_profile.h"

static void free_range(struct deltacloud_property_range *onerange)
{
  SAFE_FREE(onerange->first);
  SAFE_FREE(onerange->last);
}

int add_to_range_list(struct deltacloud_property_range **ranges,
		      const char *first, const char *last_val)
{
  struct deltacloud_property_range *onerange;

  onerange = calloc(1, sizeof(struct deltacloud_property_range));
  if (onerange == NULL)
    return -1;

  if (strdup_or_null(&onerange->first, first) < 0)
    goto error;
  if (strdup_or_null(&onerange->last, last_val) < 0)
    goto error;

  add_to_list(ranges, struct deltacloud_property_range, onerange);

  return 0;

 error:
  free_range(onerange);
  SAFE_FREE(onerange);
  return -1;
}

static int copy_range(struct deltacloud_property_range **dst,
		      struct deltacloud_property_range *curr)
{
  return add_to_range_list(dst, curr->first, curr->last);
}

static int copy_range_list(struct deltacloud_property_range **dst,
			   struct deltacloud_property_range **src)
{
  copy_list(dst, src, struct deltacloud_property_range, copy_range,
	    free_range_list);
}

void free_range_list(struct deltacloud_property_range **ranges)
{
  free_list(ranges, struct deltacloud_property_range, free_range);
}

static void free_enum(struct deltacloud_property_enum *oneenum)
{
  SAFE_FREE(oneenum->value);
}

int add_to_enum_list(struct deltacloud_property_enum **enums,
		     const char *value)
{
  struct deltacloud_property_enum *oneenum;

  oneenum = calloc(1, sizeof(struct deltacloud_property_enum));
  if (oneenum == NULL)
    return -1;

  if (strdup_or_null(&oneenum->value, value) < 0)
    goto error;

  add_to_list(enums, struct deltacloud_property_enum, oneenum);

  return 0;

 error:
  free_enum(oneenum);
  SAFE_FREE(oneenum);
  return -1;
}

static int copy_enum(struct deltacloud_property_enum **dst,
		     struct deltacloud_property_enum *curr)
{
  return add_to_enum_list(dst, curr->value);
}

static int copy_enum_list(struct deltacloud_property_enum **dst,
			  struct deltacloud_property_enum **src)
{
  copy_list(dst, src, struct deltacloud_property_enum, copy_enum,
	    free_enum_list);
}

void free_enum_list(struct deltacloud_property_enum **enums)
{
  free_list(enums, struct deltacloud_property_enum, free_enum);
}

static void free_param(struct deltacloud_property_param *param)
{
  SAFE_FREE(param->href);
  SAFE_FREE(param->method);
  SAFE_FREE(param->name);
  SAFE_FREE(param->operation);
}

int add_to_param_list(struct deltacloud_property_param **params,
		      const char *href, const char *method, const char *name,
		      const char *operation)
{
  struct deltacloud_property_param *oneparam;

  oneparam = calloc(1, sizeof(struct deltacloud_property_param));
  if (oneparam == NULL)
    return -1;

  if (strdup_or_null(&oneparam->href, href) < 0)
    goto error;
  if (strdup_or_null(&oneparam->method, method) < 0)
    goto error;
  if (strdup_or_null(&oneparam->name, name) < 0)
    goto error;
  if (strdup_or_null(&oneparam->operation, operation) < 0)
    goto error;

  add_to_list(params, struct deltacloud_property_param, oneparam);

  return 0;

 error:
  free_param(oneparam);
  SAFE_FREE(oneparam);
  return -1;
}

static int copy_param(struct deltacloud_property_param **dst,
		      struct deltacloud_property_param *curr)
{
  return add_to_param_list(dst, curr->href, curr->method, curr->name,
			   curr->operation);
}

static int copy_param_list(struct deltacloud_property_param **dst,
			   struct deltacloud_property_param **src)
{
  copy_list(dst, src, struct deltacloud_property_param, copy_param,
	    free_param_list);
}

void free_param_list(struct deltacloud_property_param **params)
{
  free_list(params, struct deltacloud_property_param, free_param);
}

static void free_prop(struct deltacloud_property *prop)
{
  SAFE_FREE(prop->kind);
  SAFE_FREE(prop->name);
  SAFE_FREE(prop->unit);
  SAFE_FREE(prop->value);
  free_param_list(&prop->params);
  free_enum_list(&prop->enums);
  free_range_list(&prop->ranges);
}

void free_property_list(struct deltacloud_property **props)
{
  free_list(props, struct deltacloud_property, free_prop);
}

int add_to_property_list(struct deltacloud_property **props, const char *kind,
			 const char *name, const char *unit, const char *value,
			 struct deltacloud_property_param *params,
			 struct deltacloud_property_enum *enums,
			 struct deltacloud_property_range *ranges)
{
  struct deltacloud_property *oneprop;

  oneprop = calloc(1, sizeof(struct deltacloud_property));
  if (oneprop == NULL)
    return -1;

  if (strdup_or_null(&oneprop->kind, kind) < 0)
    goto error;
  if (strdup_or_null(&oneprop->name, name) < 0)
    goto error;
  if (strdup_or_null(&oneprop->unit, unit) < 0)
    goto error;
  if (strdup_or_null(&oneprop->value, value) < 0)
    goto error;
  if (copy_param_list(&oneprop->params, &params) < 0)
    goto error;
  if (copy_enum_list(&oneprop->enums, &enums) < 0)
    goto error;
  if (copy_range_list(&oneprop->ranges, &ranges) < 0)
    goto error;

  add_to_list(props, struct deltacloud_property, oneprop);

  return 0;

 error:
  free_prop(oneprop);
  SAFE_FREE(oneprop);
  return -1;
}

static int copy_property(struct deltacloud_property **dst,
			 struct deltacloud_property *curr)
{
  return add_to_property_list(dst, curr->kind, curr->name, curr->unit,
			      curr->value, curr->params, curr->enums,
			      curr->ranges);
}

static int copy_property_list(struct deltacloud_property **dst,
			      struct deltacloud_property **src)
{
  copy_list(dst, src, struct deltacloud_property, copy_property,
	    free_property_list);
}

int add_to_hardware_profile_list(struct deltacloud_hardware_profile **profiles,
				 const char *id, const char *href,
				 const char *name,
				 struct deltacloud_property *props)
{
  struct deltacloud_hardware_profile *oneprofile;

  oneprofile = calloc(1, sizeof(struct deltacloud_hardware_profile));
  if (oneprofile == NULL)
    return -1;

  if (strdup_or_null(&oneprofile->id, id) < 0)
    goto error;
  if (strdup_or_null(&oneprofile->href, href) < 0)
    goto error;
  if (strdup_or_null(&oneprofile->name, name) < 0)
    goto error;
  if (copy_property_list(&oneprofile->properties, &props) < 0)
    goto error;

  add_to_list(profiles, struct deltacloud_hardware_profile, oneprofile);

  return 0;

 error:
  deltacloud_free_hardware_profile(oneprofile);
  SAFE_FREE(oneprofile);
  return -1;
}

int copy_hardware_profile(struct deltacloud_hardware_profile *dst,
			  struct deltacloud_hardware_profile *src)
{
  /* with a NULL src, we just return success.  A NULL dst is an error */
  if (src == NULL)
    return 0;
  if (dst == NULL)
    return -1;

  memset(dst, 0, sizeof(struct deltacloud_hardware_profile));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (copy_property_list(&dst->properties, &src->properties) < 0)
    goto error;
  dst->next = NULL;

  return 0;

 error:
  deltacloud_free_hardware_profile(dst);
  return -1;
}

void deltacloud_free_hardware_profile(struct deltacloud_hardware_profile *profile)
{
  if (profile == NULL)
    return;

  SAFE_FREE(profile->id);
  SAFE_FREE(profile->href);
  SAFE_FREE(profile->name);
  free_property_list(&profile->properties);
}

void deltacloud_free_hardware_profile_list(struct deltacloud_hardware_profile **profiles)
{
  free_list(profiles, struct deltacloud_hardware_profile,
	    deltacloud_free_hardware_profile);
}
