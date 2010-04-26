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
  struct deltacloud_property_range *onerange, *curr, *last;

  onerange = malloc(sizeof(struct deltacloud_property_range));
  if (onerange == NULL)
    return -1;

  memset(onerange, 0, sizeof(struct deltacloud_property_range));

  if (strdup_or_null(&onerange->first, first) < 0)
    goto error;
  if (strdup_or_null(&onerange->last, last_val) < 0)
    goto error;

  onerange->next = NULL;

  if (*ranges == NULL)
    /* First element in the list */
    *ranges = onerange;
  else {
    curr = *ranges;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = onerange;
  }

  return 0;

 error:
  free_range(onerange);
  SAFE_FREE(onerange);
  return -1;
}

static void print_range_list(struct deltacloud_property_range **ranges,
			     FILE *stream)
{
  struct deltacloud_property_range *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *ranges;
  while (curr != NULL) {
    fprintf(stream, "  range first: %s\n", curr->first);
    fprintf(stream, "  range last: %s\n", curr->last);
    curr = curr->next;
  }
}

static int copy_range_list(struct deltacloud_property_range **dst,
			   struct deltacloud_property_range **src)
{
  struct deltacloud_property_range *curr;

  *dst = NULL;

  curr = *src;
  while (curr != NULL) {
    if (add_to_range_list(dst, curr->first, curr->last) < 0)
      goto error;
    curr = curr->next;
  }

  return 0;

 error:
  free_range_list(dst);
  return -1;
}

void free_range_list(struct deltacloud_property_range **ranges)
{
  struct deltacloud_property_range *curr, *next;

  curr = *ranges;
  while (curr != NULL) {
    next = curr->next;
    free_range(curr);
    SAFE_FREE(curr);
    curr = next;
  }

  *ranges = NULL;
}

static void free_enum(struct deltacloud_property_enum *oneenum)
{
  SAFE_FREE(oneenum->value);
}

int add_to_enum_list(struct deltacloud_property_enum **enums,
		     const char *value)
{
  struct deltacloud_property_enum *oneenum, *curr, *last;

  oneenum = malloc(sizeof(struct deltacloud_property_enum));
  if (oneenum == NULL)
    return -1;

  memset(oneenum, 0, sizeof(struct deltacloud_property_enum));

  if (strdup_or_null(&oneenum->value, value) < 0)
    goto error;

  oneenum->next = NULL;

  if (*enums == NULL)
    /* First element in the list */
    *enums = oneenum;
  else {
    curr = *enums;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneenum;
  }

  return 0;

 error:
  free_enum(oneenum);
  SAFE_FREE(oneenum);
  return -1;
}

static void print_enum_list(struct deltacloud_property_enum **enums,
			    FILE *stream)
{
  struct deltacloud_property_enum *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *enums;
  while (curr != NULL) {
    fprintf(stream, "  enum value: %s\n", curr->value);
    curr = curr->next;
  }
}

static int copy_enum_list(struct deltacloud_property_enum **dst,
			  struct deltacloud_property_enum **src)
{
  struct deltacloud_property_enum *curr;

  *dst = NULL;

  curr = *src;
  while (curr != NULL) {
    if (add_to_enum_list(dst, curr->value) < 0)
      goto error;
    curr = curr->next;
  }

  return 0;

 error:
  free_enum_list(dst);
  return -1;
}

void free_enum_list(struct deltacloud_property_enum **enums)
{
  struct deltacloud_property_enum *curr, *next;

  curr = *enums;
  while (curr != NULL) {
    next = curr->next;
    free_enum(curr);
    SAFE_FREE(curr);
    curr = next;
  }

  *enums = NULL;
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
  struct deltacloud_property_param *oneparam, *curr, *last;

  oneparam = malloc(sizeof(struct deltacloud_property_param));
  if (oneparam == NULL)
    return -1;

  memset(oneparam, 0, sizeof(struct deltacloud_property_param));

  if (strdup_or_null(&oneparam->href, href) < 0)
    goto error;
  if (strdup_or_null(&oneparam->method, method) < 0)
    goto error;
  if (strdup_or_null(&oneparam->name, name) < 0)
    goto error;
  if (strdup_or_null(&oneparam->operation, operation) < 0)
    goto error;

  oneparam->next = NULL;

  if (*params == NULL)
    /* First element in the list */
    *params = oneparam;
  else {
    curr = *params;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneparam;
  }

  return 0;

 error:
  free_param(oneparam);
  SAFE_FREE(oneparam);
  return -1;
}

static void print_param_list(struct deltacloud_property_param **params,
			     FILE *stream)
{
  struct deltacloud_property_param *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *params;
  while (curr != NULL) {
    fprintf(stream, "  param href: %s\n", curr->href);
    fprintf(stream, "  param method: %s\n", curr->method);
    fprintf(stream, "  param name: %s\n", curr->name);
    fprintf(stream, "  param operation: %s\n", curr->operation);
    curr = curr->next;
  }
}

static int copy_param_list(struct deltacloud_property_param **dst,
			   struct deltacloud_property_param **src)
{
  struct deltacloud_property_param *curr;

  *dst = NULL;

  curr = *src;
  while (curr != NULL) {
    if (add_to_param_list(dst, curr->href, curr->method, curr->name,
			  curr->operation) < 0)
      goto error;
    curr = curr->next;
  }

  return 0;

 error:
  free_param_list(dst);
  return -1;
}

void free_param_list(struct deltacloud_property_param **params)
{
  struct deltacloud_property_param *curr, *next;

  curr = *params;
  while (curr != NULL) {
    next = curr->next;
    free_param(curr);
    SAFE_FREE(curr);
    curr = next;
  }

  *params = NULL;
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
  struct deltacloud_property *curr, *next;

  curr = *props;
  while (curr != NULL) {
    next = curr->next;
    free_prop(curr);
    SAFE_FREE(curr);
    curr = next;
  }

  *props = NULL;
}

int add_to_property_list(struct deltacloud_property **props, const char *kind,
			 const char *name, const char *unit, const char *value,
			 struct deltacloud_property_param *params,
			 struct deltacloud_property_enum *enums,
			 struct deltacloud_property_range *ranges)
{
  struct deltacloud_property *oneprop, *curr, *last;

  oneprop = malloc(sizeof(struct deltacloud_property));
  if (oneprop == NULL)
    return -1;

  memset(oneprop, 0, sizeof(struct deltacloud_property));

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

  oneprop->next = NULL;

  if (*props == NULL)
    /* First element in the list */
    *props = oneprop;
  else {
    curr = *props;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneprop;
  }

  return 0;

 error:
  free_prop(oneprop);
  SAFE_FREE(oneprop);
  return -1;
}

static void print_property_list(struct deltacloud_property **props,
				FILE *stream)
{
  struct deltacloud_property *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *props;
  while (curr != NULL) {
    fprintf(stream, " kind: %s\n", curr->kind);
    fprintf(stream, " name: %s\n", curr->name);
    fprintf(stream, " unit: %s\n", curr->unit);
    fprintf(stream, " value: %s\n", curr->value);
    print_param_list(&curr->params, stream);
    print_enum_list(&curr->enums, stream);
    print_range_list(&curr->ranges, stream);
    curr = curr->next;
  }
}

static int copy_property_list(struct deltacloud_property **dst,
			      struct deltacloud_property **src)
{
  struct deltacloud_property *curr;

  *dst = NULL;

  curr = *src;
  while (curr != NULL) {
    if (add_to_property_list(dst, curr->kind, curr->name, curr->unit,
			     curr->value, curr->params, curr->enums,
			     curr->ranges) < 0)
      goto error;
    curr = curr->next;
  }

  return 0;

 error:
  free_property_list(dst);
  return -1;
}

static void deltacloud_free_hardware_profile(struct deltacloud_hardware_profile *profile);
int add_to_hardware_profile_list(struct deltacloud_hardware_profile **profiles,
				 const char *id, const char *href,
				 struct deltacloud_property *props)
{
  struct deltacloud_hardware_profile *oneprofile, *curr, *last;

  oneprofile = malloc(sizeof(struct deltacloud_hardware_profile));
  if (oneprofile == NULL)
    return -1;

  memset(oneprofile, 0, sizeof(struct deltacloud_hardware_profile));

  if (strdup_or_null(&oneprofile->id, id) < 0)
    goto error;
  if (strdup_or_null(&oneprofile->href, href) < 0)
    goto error;
  if (copy_property_list(&oneprofile->properties, &props) < 0)
    goto error;

  oneprofile->next = NULL;

  if (*profiles == NULL)
    /* First element in the list */
    *profiles = oneprofile;
  else {
    curr = *profiles;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneprofile;
  }

  return 0;

 error:
  deltacloud_free_hardware_profile(oneprofile);
  SAFE_FREE(oneprofile);
  return -1;
}

void deltacloud_print_hardware_profile(struct deltacloud_hardware_profile *profile,
				       FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "ID: %s\n", profile->id);
  fprintf(stream, "HREF: %s\n", profile->href);
  print_property_list(&profile->properties, stream);
}

void deltacloud_print_hardware_profile_list(struct deltacloud_hardware_profile **profiles,
					    FILE *stream)
{
  struct deltacloud_hardware_profile *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *profiles;
  while (curr != NULL) {
    deltacloud_print_hardware_profile(curr, NULL);
    curr = curr->next;
  }
}

static void deltacloud_free_hardware_profile(struct deltacloud_hardware_profile *profile)
{
  SAFE_FREE(profile->id);
  SAFE_FREE(profile->href);
  free_property_list(&profile->properties);
}

void deltacloud_free_hardware_profile_list(struct deltacloud_hardware_profile **profiles)
{
  struct deltacloud_hardware_profile *curr, *next;

  curr = *profiles;
  while (curr != NULL) {
    next = curr->next;
    deltacloud_free_hardware_profile(curr);
    SAFE_FREE(curr);
    curr = next;
  }

  *profiles = NULL;
}
