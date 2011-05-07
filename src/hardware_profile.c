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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "libdeltacloud.h"

static void free_range(struct deltacloud_property_range *onerange)
{
  SAFE_FREE(onerange->first);
  SAFE_FREE(onerange->last);
}

static int add_to_range_list(struct deltacloud_property_range **ranges,
			     struct deltacloud_property_range *range)
{
  struct deltacloud_property_range *onerange;

  onerange = calloc(1, sizeof(struct deltacloud_property_range));
  if (onerange == NULL)
    return -1;

  if (strdup_or_null(&onerange->first, range->first) < 0)
    goto error;
  if (strdup_or_null(&onerange->last, range->last) < 0)
    goto error;

  add_to_list(ranges, struct deltacloud_property_range, onerange);

  return 0;

 error:
  free_range(onerange);
  SAFE_FREE(onerange);
  return -1;
}

static void free_range_list(struct deltacloud_property_range **ranges)
{
  free_list(ranges, struct deltacloud_property_range, free_range);
}

static int copy_range_list(struct deltacloud_property_range **dst,
			   struct deltacloud_property_range **src)
{
  copy_list(dst, src, struct deltacloud_property_range, add_to_range_list,
	    free_range_list);
}

static void free_enum(struct deltacloud_property_enum *oneenum)
{
  SAFE_FREE(oneenum->value);
}

static int add_to_enum_list(struct deltacloud_property_enum **enums,
			    struct deltacloud_property_enum *inenum)
{
  struct deltacloud_property_enum *oneenum;

  oneenum = calloc(1, sizeof(struct deltacloud_property_enum));
  if (oneenum == NULL)
    return -1;

  if (strdup_or_null(&oneenum->value, inenum->value) < 0)
    goto error;

  add_to_list(enums, struct deltacloud_property_enum, oneenum);

  return 0;

 error:
  free_enum(oneenum);
  SAFE_FREE(oneenum);
  return -1;
}

static void free_enum_list(struct deltacloud_property_enum **enums)
{
  free_list(enums, struct deltacloud_property_enum, free_enum);
}

static int copy_enum_list(struct deltacloud_property_enum **dst,
			  struct deltacloud_property_enum **src)
{
  copy_list(dst, src, struct deltacloud_property_enum, add_to_enum_list,
	    free_enum_list);
}

static void free_param(struct deltacloud_property_param *param)
{
  SAFE_FREE(param->href);
  SAFE_FREE(param->method);
  SAFE_FREE(param->name);
  SAFE_FREE(param->operation);
}

static int add_to_param_list(struct deltacloud_property_param **params,
		      struct deltacloud_property_param *param)
{
  struct deltacloud_property_param *oneparam;

  oneparam = calloc(1, sizeof(struct deltacloud_property_param));
  if (oneparam == NULL)
    return -1;

  if (strdup_or_null(&oneparam->href, param->href) < 0)
    goto error;
  if (strdup_or_null(&oneparam->method, param->method) < 0)
    goto error;
  if (strdup_or_null(&oneparam->name, param->name) < 0)
    goto error;
  if (strdup_or_null(&oneparam->operation, param->operation) < 0)
    goto error;

  add_to_list(params, struct deltacloud_property_param, oneparam);

  return 0;

 error:
  free_param(oneparam);
  SAFE_FREE(oneparam);
  return -1;
}

static void free_param_list(struct deltacloud_property_param **params)
{
  free_list(params, struct deltacloud_property_param, free_param);
}

static int copy_param_list(struct deltacloud_property_param **dst,
			   struct deltacloud_property_param **src)
{
  copy_list(dst, src, struct deltacloud_property_param, add_to_param_list,
	    free_param_list);
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

static void free_property_list(struct deltacloud_property **props)
{
  free_list(props, struct deltacloud_property, free_prop);
}

static int add_to_property_list(struct deltacloud_property **props,
				struct deltacloud_property *prop)
{
  struct deltacloud_property *oneprop;

  oneprop = calloc(1, sizeof(struct deltacloud_property));
  if (oneprop == NULL)
    return -1;

  if (strdup_or_null(&oneprop->kind, prop->kind) < 0)
    goto error;
  if (strdup_or_null(&oneprop->name, prop->name) < 0)
    goto error;
  if (strdup_or_null(&oneprop->unit, prop->unit) < 0)
    goto error;
  if (strdup_or_null(&oneprop->value, prop->value) < 0)
    goto error;
  if (copy_param_list(&oneprop->params, &prop->params) < 0)
    goto error;
  if (copy_enum_list(&oneprop->enums, &prop->enums) < 0)
    goto error;
  if (copy_range_list(&oneprop->ranges, &prop->ranges) < 0)
    goto error;

  add_to_list(props, struct deltacloud_property, oneprop);

  return 0;

 error:
  free_prop(oneprop);
  SAFE_FREE(oneprop);
  return -1;
}

static int copy_property_list(struct deltacloud_property **dst,
			      struct deltacloud_property **src)
{
  copy_list(dst, src, struct deltacloud_property, add_to_property_list,
	    free_property_list);
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

  return 0;

 error:
  deltacloud_free_hardware_profile(dst);
  return -1;
}

static int parse_hwp_params_enums_ranges(xmlNodePtr property,
					 struct deltacloud_property *prop)
{
  struct deltacloud_property_param *thisparam;
  struct deltacloud_property_enum *thisenum;
  struct deltacloud_property_range *thisrange;
  xmlNodePtr enum_cur;
  int ret = -1;

  while (property != NULL) {
    if (property->type == XML_ELEMENT_NODE) {
      if (STREQ((const char *)property->name, "param")) {

	thisparam = calloc(1, sizeof(struct deltacloud_property_param));
	if (thisparam == NULL) {
	  oom_error();
	  goto cleanup;
	}

	thisparam->href = (char *)xmlGetProp(property, BAD_CAST "href");
	thisparam->method = (char *)xmlGetProp(property, BAD_CAST "method");
	thisparam->name = (char *)xmlGetProp(property, BAD_CAST "name");
	thisparam->operation = (char *)xmlGetProp(property,
						  BAD_CAST "operation");

	/* add_to_list can't fail */
	add_to_list(&prop->params, struct deltacloud_property_param, thisparam);
      }
      else if(STREQ((const char *)property->name, "enum")) {
	enum_cur = property->children;
	while (enum_cur != NULL) {
	  if (enum_cur->type == XML_ELEMENT_NODE &&
	      STREQ((const char *)enum_cur->name, "entry")) {

	    thisenum = calloc(1, sizeof(struct deltacloud_property_enum));
	    if (thisenum == NULL) {
	      oom_error();
	      goto cleanup;
	    }

	    thisenum->value = (char *)xmlGetProp(enum_cur, BAD_CAST "value");

	    /* add_to_list can't fail */
	    add_to_list(&prop->enums, struct deltacloud_property_enum,
			thisenum);
	  }

	  enum_cur = enum_cur->next;
	}
      }
      else if (STREQ((const char *)property->name, "range")) {
	thisrange = calloc(1, sizeof(struct deltacloud_property_range));
	if (thisrange == NULL) {
	  oom_error();
	  goto cleanup;
	}
	thisrange->first = (char *)xmlGetProp(property, BAD_CAST "first");
	thisrange->last = (char *)xmlGetProp(property, BAD_CAST "last");

	/* add_to_list can't fail */
	add_to_list(&prop->ranges, struct deltacloud_property_range, thisrange);
      }
    }
    property = property->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0) {
    free_param_list(&prop->params);
    free_enum_list(&prop->enums);
    free_range_list(&prop->ranges);
  }

  return ret;
}

static int parse_hardware_profile_properties(xmlNodePtr hwp,
					     struct deltacloud_property **props)
{
  xmlNodePtr profile_cur;
  struct deltacloud_property thisprop;
  int listret;
  int ret = -1;

  memset(&thisprop, 0, sizeof(struct deltacloud_property));

  profile_cur = hwp->children;

  while (profile_cur != NULL) {
    if (profile_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)profile_cur->name, "property")) {
      thisprop.kind = (char *)xmlGetProp(profile_cur, BAD_CAST "kind");
      thisprop.name = (char *)xmlGetProp(profile_cur, BAD_CAST "name");
      thisprop.unit = (char *)xmlGetProp(profile_cur, BAD_CAST "unit");
      thisprop.value = (char *)xmlGetProp(profile_cur, BAD_CAST "value");

      if (parse_hwp_params_enums_ranges(profile_cur->children, &thisprop) < 0) {
	/* parse_hwp_params_enums_ranges already set the error */
	free_prop(&thisprop);
	goto cleanup;
      }

      listret = add_to_property_list(props, &thisprop);
      free_prop(&thisprop);
      if (listret < 0) {
	oom_error();
	goto cleanup;
      }
    }

    profile_cur = profile_cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_property_list(props);
  return ret;
}

int parse_hardware_profile_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			       void **data)
{
  struct deltacloud_hardware_profile **profiles = (struct deltacloud_hardware_profile **)data;
  struct deltacloud_hardware_profile *thishwp;
  xmlNodePtr oldnode;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "hardware_profile")) {

      ctxt->node = cur;

      thishwp = calloc(1, sizeof(struct deltacloud_hardware_profile));
      if (thishwp == NULL) {
	oom_error();
	goto cleanup;
      }

      thishwp->href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thishwp->id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thishwp->name = getXPathString("string(./name)", ctxt);

      if (parse_hardware_profile_properties(cur, &(thishwp->properties)) < 0) {
	/* parse_hardware_profile_properties already set the error */
	deltacloud_free_hardware_profile(thishwp);
	goto cleanup;
      }

      add_to_list(profiles, struct deltacloud_hardware_profile, thishwp);
    }

    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_hardware_profile_list(profiles);

  return ret;
}

static int parse_one_hwp(const char *data, void *output)
{
  int ret = -1;
  struct deltacloud_hardware_profile *newprofile = (struct deltacloud_hardware_profile *)output;
  struct deltacloud_hardware_profile *tmpprofile = NULL;

  if (parse_xml(data, "hardware_profile", (void **)&tmpprofile,
		parse_hardware_profile_xml, 0) < 0)
    goto cleanup;

  if (copy_hardware_profile(newprofile, tmpprofile) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_hardware_profile_list(&tmpprofile);

  return ret;
}

int deltacloud_get_hardware_profiles(struct deltacloud_api *api,
				     struct deltacloud_hardware_profile **profiles)
{
  return internal_get(api, "hardware_profiles", "hardware_profiles",
		      parse_hardware_profile_xml, (void **)profiles);
}

int deltacloud_get_hardware_profile_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_hardware_profile *profile)
{
  return internal_get_by_id(api, id, "hardware_profiles", parse_one_hwp,
			    profile);
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
