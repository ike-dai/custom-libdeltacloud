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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "libdeltacloud.h"

static void free_provider(struct deltacloud_driver_provider *provider)
{
  SAFE_FREE(provider->id);
}

static void free_provider_list(struct deltacloud_driver_provider **providers)
{
  free_list(providers, struct deltacloud_driver_provider, free_provider);
}

static int add_to_provider_list(struct deltacloud_driver_provider **providers,
				struct deltacloud_driver_provider *provider)
{
  struct deltacloud_driver_provider *oneprovider;

  oneprovider = calloc(1, sizeof(struct deltacloud_driver_provider));
  if (oneprovider == NULL)
    return -1;

  if (strdup_or_null(&oneprovider->id, provider->id) < 0)
    goto error;

  add_to_list(providers, struct deltacloud_driver_provider, oneprovider);

  return 0;

 error:
  free_provider(oneprovider);
  SAFE_FREE(oneprovider);
  return -1;
}

static int parse_driver_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			    void **data)
{
  struct deltacloud_driver **drivers = (struct deltacloud_driver **)data;
  struct deltacloud_driver *thisdriver;
  struct deltacloud_driver_provider thisprovider;
  int ret = -1;
  xmlNodePtr oldnode, providernode;
  int listret;

  oldnode = ctxt->node;

  memset(&thisprovider, 0, sizeof(struct deltacloud_driver_provider));

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "driver")) {

      ctxt->node = cur;

      thisdriver = calloc(1, sizeof(struct deltacloud_driver));
      if (thisdriver == NULL) {
	oom_error();
	goto cleanup;
      }

      thisdriver->href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thisdriver->id = (char *)xmlGetProp(cur, BAD_CAST "id");
      thisdriver->name = getXPathString("string(./name)", ctxt);

      providernode = cur->children;
      while (providernode != NULL) {
	if (providernode->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)providernode->name, "provider")) {

	  thisprovider.id = (char *)xmlGetProp(providernode, BAD_CAST "id");

	  listret = add_to_provider_list(&(thisdriver->providers),
					 &thisprovider);
	  free_provider(&thisprovider);
	  if (listret < 0) {
	    free_provider_list(&(thisdriver->providers));
	    oom_error();
	    goto cleanup;
	  }
	}
	providernode = providernode->next;
      }

      /* add_to_list can't fail */
      add_to_list(drivers, struct deltacloud_driver, thisdriver);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_driver_list(drivers);

  return ret;
}

static int copy_provider_list(struct deltacloud_driver_provider **dst,
			      struct deltacloud_driver_provider **src)
{
  copy_list(dst, src, struct deltacloud_driver_provider, add_to_provider_list,
	    free_provider_list);
}

static int copy_driver(struct deltacloud_driver *dst,
		       struct deltacloud_driver *src)
{
  /* with a NULL src, we just return success.  A NULL dst is an error */
  if (src == NULL)
    return 0;
  if (dst == NULL)
    return -1;

  memset(dst, 0, sizeof(struct deltacloud_driver));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->name, src->name) < 0)
    goto error;
  if (copy_provider_list(&dst->providers, &src->providers) < 0)
    goto error;

  return 0;

 error:
  deltacloud_free_driver(dst);
  return -1;
}

static int parse_one_driver(const char *data, void *output)
{
  int ret = -1;
  struct deltacloud_driver *newdriver = (struct deltacloud_driver *)output;
  struct deltacloud_driver *tmpdriver = NULL;

  if (parse_xml(data, "driver", (void **)&tmpdriver, parse_driver_xml, 0) < 0)
    goto cleanup;

  if (copy_driver(newdriver, tmpdriver) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_driver_list(&tmpdriver);

  return ret;
}

int deltacloud_get_drivers(struct deltacloud_api *api,
			   struct deltacloud_driver **drivers)
{
  return internal_get(api, "drivers", "drivers", parse_driver_xml,
		      (void **)drivers);
}

int deltacloud_get_driver_by_id(struct deltacloud_api *api, const char *id,
				struct deltacloud_driver *driver)
{
  return internal_get_by_id(api, id, "drivers", parse_one_driver, driver);
}

void deltacloud_free_driver(struct deltacloud_driver *driver)
{
  if (driver == NULL)
    return;

  SAFE_FREE(driver->href);
  SAFE_FREE(driver->id);
  SAFE_FREE(driver->name);
  free_provider_list(&driver->providers);
}

void deltacloud_free_driver_list(struct deltacloud_driver **drivers)
{
  free_list(drivers, struct deltacloud_driver, deltacloud_free_driver);
}
