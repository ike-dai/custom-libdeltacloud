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
#include "driver.h"

/** @file */

static void free_provider(struct deltacloud_driver_provider *provider)
{
  SAFE_FREE(provider->id);
}

static void free_provider_list(struct deltacloud_driver_provider **providers)
{
  free_list(providers, struct deltacloud_driver_provider, free_provider);
}

static int parse_one_driver(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			    void *output)
{
  struct deltacloud_driver *thisdriver = (struct deltacloud_driver *)output;
  xmlNodePtr providernode;
  struct deltacloud_driver_provider *thisprovider;

  memset(thisdriver, 0, sizeof(struct deltacloud_driver));

  thisdriver->href = (char *)xmlGetProp(cur, BAD_CAST "href");
  thisdriver->id = (char *)xmlGetProp(cur, BAD_CAST "id");
  thisdriver->name = getXPathString("string(./name)", ctxt);

  providernode = cur->children;
  while (providernode != NULL) {
    if (providernode->type == XML_ELEMENT_NODE &&
	STREQ((const char *)providernode->name, "provider")) {

      thisprovider = calloc(1, sizeof(struct deltacloud_driver_provider));
      if (thisprovider == NULL) {
	oom_error();
	deltacloud_free_driver(thisdriver);
	return -1;
      }

      thisprovider->id = (char *)xmlGetProp(providernode, BAD_CAST "id");

      add_to_list(&thisdriver->providers, struct deltacloud_driver_provider,
		  thisprovider);
    }
    providernode = providernode->next;
  }

  return 0;
}

static int parse_driver_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			    void **data)
{
  struct deltacloud_driver **drivers = (struct deltacloud_driver **)data;
  struct deltacloud_driver *thisdriver;
  int ret = -1;
  xmlNodePtr oldnode;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "driver")) {

      ctxt->node = cur;

      thisdriver = calloc(1, sizeof(struct deltacloud_driver));
      if (thisdriver == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_driver(cur, ctxt, thisdriver) < 0) {
	/* parse_one_driver is expected to have set its own error */
	SAFE_FREE(thisdriver);
	goto cleanup;
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

/**
 * A function to get a linked list of all of the drivers supported.  The caller
 * is expected to free the list using deltacloud_free_driver_list().
 * @param[in] api The deltacloud_api structure representing this connection
 * @param[out] drivers A pointer to the deltacloud_driver structure to hold
 *                     the list of drivers
 * @returns 0 on success, -1 on error
 */
int deltacloud_get_drivers(struct deltacloud_api *api,
			   struct deltacloud_driver **drivers)
{
  return internal_get(api, "drivers", "drivers", parse_driver_xml,
		      (void **)drivers);
}

/**
 * A function to look up a particular driver by id.  The caller is expected
 * to free the deltacloud_driver structure using deltacloud_free_driver().
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] id The driver ID to look for
 * @param[out] driver The deltacloud_driver structure to fill in if the ID
 *                    is found
 * @returns 0 on success, -1 if the driver cannot be found or on error
 */
int deltacloud_get_driver_by_id(struct deltacloud_api *api, const char *id,
				struct deltacloud_driver *driver)
{
  return internal_get_by_id(api, id, "drivers", "driver", parse_one_driver,
			    driver);
}

/**
 * A function to free a deltacloud_driver structure initially allocated
 * by deltacloud_get_driver_by_id().
 * @param[in] driver The deltacloud_driver structure representing the driver
 */
void deltacloud_free_driver(struct deltacloud_driver *driver)
{
  if (driver == NULL)
    return;

  SAFE_FREE(driver->href);
  SAFE_FREE(driver->id);
  SAFE_FREE(driver->name);
  free_provider_list(&driver->providers);
}

/**
 * A function to free a list of deltacloud_driver structures initially allocated
 * by deltacloud_get_drivers().
 * @param[in] drivers The pointer to the head of the deltacloud_driver list
 */
void deltacloud_free_driver_list(struct deltacloud_driver **drivers)
{
  free_list(drivers, struct deltacloud_driver, deltacloud_free_driver);
}
