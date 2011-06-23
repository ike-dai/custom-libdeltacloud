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
#include <string.h>
#include "libdeltacloud.h"
#include "test_common.h"

static void print_provider(struct deltacloud_driver_provider *provider)
{
  fprintf(stderr, "\tProvider: %s\n", provider->id);
}

static void print_driver(struct deltacloud_driver *driver)
{
  struct deltacloud_driver_provider *provider;

  fprintf(stderr, "Driver %s\n", driver->id);
  fprintf(stderr, "\tHref: %s\n", driver->href);
  fprintf(stderr, "\tName: %s\n", driver->name);
  deltacloud_for_each(provider, driver->providers)
    print_provider(provider);
}

static void print_driver_list(struct deltacloud_driver *drivers)
{
  struct deltacloud_driver *driver;

  deltacloud_for_each(driver, drivers)
    print_driver(driver);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_api zeroapi;
  struct deltacloud_driver *drivers = NULL;
  struct deltacloud_driver driver;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  memset(&zeroapi, 0, sizeof(struct deltacloud_api));

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }

  /* test out deltacloud_supports_drivers */
  if (deltacloud_supports_drivers(NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_drivers to fail with NULL api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_drivers(&zeroapi) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_drivers to fail with uninitialized api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_drivers(&api)) {

    /* test out deltacloud_get_drivers */
    if (deltacloud_get_drivers(NULL, &drivers) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_drivers to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_drivers(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_drivers to fail with NULL drivers, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_drivers(&zeroapi, &drivers) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_drivers to fail with unintialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_drivers(&api, &drivers) < 0) {
      fprintf(stderr, "Failed to get drivers: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_driver_list(drivers);

    if (drivers != NULL) {

      /* test out deltacloud_get_driver_by_id */
      if (deltacloud_get_driver_by_id(NULL, drivers->id, &driver) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_driver_by_id to fail with NULL api, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_driver_by_id(&api, NULL, &driver) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_driver_by_id to fail with NULL id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_driver_by_id(&api, drivers->id, NULL) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_driver_by_id to fail with NULL driver, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_driver_by_id(&api, "bogus_id", &driver) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_driver_by_id to fail with bogus id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_driver_by_id(&zeroapi, drivers->id, &driver) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_driver_by_id to fail with unintialized api, but succeeded\n");
	goto cleanup;
      }

      /* here we use the first driver from the list above */
      if (deltacloud_get_driver_by_id(&api, drivers->id, &driver) < 0) {
	fprintf(stderr, "Failed to get driver: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }
      print_driver(&driver);
      deltacloud_free_driver(&driver);
    }
  }
  else
    fprintf(stderr, "Drivers are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free_driver_list(&drivers);

  deltacloud_free(&api);

  return ret;
}
