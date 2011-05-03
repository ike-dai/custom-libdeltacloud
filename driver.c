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

void free_provider(struct deltacloud_driver_provider *provider)
{
  SAFE_FREE(provider->id);
}

int add_to_provider_list(struct deltacloud_driver_provider **providers,
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

static int copy_provider_list(struct deltacloud_driver_provider **dst,
			      struct deltacloud_driver_provider **src)
{
  copy_list(dst, src, struct deltacloud_driver_provider, add_to_provider_list,
	    free_provider_list);
}

void free_provider_list(struct deltacloud_driver_provider **providers)
{
  free_list(providers, struct deltacloud_driver_provider, free_provider);
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

int copy_driver(struct deltacloud_driver *dst, struct deltacloud_driver *src)
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

int add_to_driver_list(struct deltacloud_driver **drivers,
		       struct deltacloud_driver *driver)
{
  struct deltacloud_driver *onedriver;

  onedriver = calloc(1, sizeof(struct deltacloud_driver));
  if (onedriver == NULL)
    return -1;

  if (copy_driver(onedriver, driver) < 0)
    goto error;

  add_to_list(drivers, struct deltacloud_driver, onedriver);

  return 0;

 error:
  deltacloud_free_driver(onedriver);
  SAFE_FREE(onedriver);
  return -1;
}

void deltacloud_free_driver_list(struct deltacloud_driver **drivers)
{
  free_list(drivers, struct deltacloud_driver, deltacloud_free_driver);
}
