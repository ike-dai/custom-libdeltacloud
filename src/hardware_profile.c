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
#include "hardware_profile.h"

/** @file */

static void free_range(struct deltacloud_property_range *onerange)
{
  SAFE_FREE(onerange->first);
  SAFE_FREE(onerange->last);
}

static void free_range_list(struct deltacloud_property_range **ranges)
{
  free_list(ranges, struct deltacloud_property_range, free_range);
}

static void free_enum(struct deltacloud_property_enum *oneenum)
{
  SAFE_FREE(oneenum->value);
}

static void free_enum_list(struct deltacloud_property_enum **enums)
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

static void free_param_list(struct deltacloud_property_param **params)
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

static void free_property_list(struct deltacloud_property **props)
{
  free_list(props, struct deltacloud_property, free_prop);
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
  struct deltacloud_property *thisprop;
  int ret = -1;

  profile_cur = hwp->children;

  while (profile_cur != NULL) {
    if (profile_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)profile_cur->name, "property")) {

      thisprop = calloc(1, sizeof(struct deltacloud_property));
      if (thisprop == NULL) {
	oom_error();
	goto cleanup;
      }

      thisprop->kind = (char *)xmlGetProp(profile_cur, BAD_CAST "kind");
      thisprop->name = (char *)xmlGetProp(profile_cur, BAD_CAST "name");
      thisprop->unit = (char *)xmlGetProp(profile_cur, BAD_CAST "unit");
      thisprop->value = (char *)xmlGetProp(profile_cur, BAD_CAST "value");

      if (parse_hwp_params_enums_ranges(profile_cur->children, thisprop) < 0) {
	/* parse_hwp_params_enums_ranges already set the error */
	free_prop(thisprop);
	SAFE_FREE(thisprop);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(props, struct deltacloud_property, thisprop);
    }

    profile_cur = profile_cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_property_list(props);
  return ret;
}

/** @cond INTERNAL */
int parse_one_hardware_profile(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			       void *output)
{
  struct deltacloud_hardware_profile *thishwp = (struct deltacloud_hardware_profile *)output;

  memset(thishwp, 0, sizeof(struct deltacloud_hardware_profile));

  thishwp->href = (char *)xmlGetProp(cur, BAD_CAST "href");
  thishwp->id = (char *)xmlGetProp(cur, BAD_CAST "id");
  thishwp->name = getXPathString("string(./name)", ctxt);

  if (parse_hardware_profile_properties(cur, &(thishwp->properties)) < 0) {
    /* parse_hardware_profile_properties already set the error */
    deltacloud_free_hardware_profile(thishwp);
    return -1;
  }

  return 0;
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

      if (parse_one_hardware_profile(cur, ctxt, thishwp) < 0) {
	/* parse_one_hardware_profile is expected to have set its own error */
	SAFE_FREE(thishwp);
	goto cleanup;
      }

      /* add_to_list can't fail */
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
/** @endcond */

/**
 * A function to get a linked list of all of the hardware profiles supported.
 * The caller is expected to free the list using
 * deltacloud_free_hardware_profile_list().
 * @param[in] api The deltacloud_api structure representing this connection
 * @param[out] profiles A pointer to the deltacloud_hardware_profile structure
 *                      to hold the list of hardware profiles
 * @returns 0 on success, -1 on error
 */
int deltacloud_get_hardware_profiles(struct deltacloud_api *api,
				     struct deltacloud_hardware_profile **profiles)
{
  return internal_get(api, "hardware_profiles", "hardware_profiles",
		      parse_hardware_profile_xml, (void **)profiles);
}

/**
 * A function to look up a particular hardware profile by id.  The caller is
 * expected to free the deltacloud_hardware_profile structure using
 * deltacloud_free_hardware_profile().
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] id The hardware_profile ID to look for
 * @param[out] profile The deltacloud_hardware_profile structure to fill in if
 *                     the ID is found
 * @returns 0 on success, -1 if the hardware profile cannot be found or on error
 */
int deltacloud_get_hardware_profile_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_hardware_profile *profile)
{
  return internal_get_by_id(api, id, "hardware_profiles", "hardware_profile",
			    parse_one_hardware_profile, profile);
}

/**
 * A function to free a deltacloud_hardware_profile structure initially
 * allocated by deltacloud_get_hardware_profile_by_id().
 * @param[in] profile The deltacloud_hardware_profile structure representing
 *                    the hardware profile
 */
void deltacloud_free_hardware_profile(struct deltacloud_hardware_profile *profile)
{
  if (profile == NULL)
    return;

  SAFE_FREE(profile->id);
  SAFE_FREE(profile->href);
  SAFE_FREE(profile->name);
  free_property_list(&profile->properties);
}

/**
 * A function to free a list of deltacloud_hardware_profile structures
 * initially allocated by deltacloud_get_hardware_profiles().
 * @param[in] profiles The pointer to the head of the
 *                     deltacloud_hardware_profile list
 */
void deltacloud_free_hardware_profile_list(struct deltacloud_hardware_profile **profiles)
{
  free_list(profiles, struct deltacloud_hardware_profile,
	    deltacloud_free_hardware_profile);
}
