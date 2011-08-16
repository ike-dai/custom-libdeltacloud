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

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_create_parameter stackparams[2];
  struct deltacloud_create_parameter *heapparams[2];
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }

  if (deltacloud_prepare_parameter(NULL, "name", "foo") >= 0) {
    fprintf(stderr, "Expected deltacloud_prepare_parameter to fail with NULL parameter, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_prepare_parameter(&stackparams[0], NULL, "foo") >= 0) {
    fprintf(stderr, "Expected deltacloud_prepare_parameter to fail with NULL name, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_prepare_parameter(&stackparams[0], "name", NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_prepare_parameter to fail with NULL name, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_prepare_parameter(&stackparams[0], "name", "foo") < 0) {
    fprintf(stderr, "Failed to prepare stack parameter 0: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  if (deltacloud_prepare_parameter(&stackparams[1], "keyname", "chris") < 0) {
    deltacloud_free_parameter_value(&stackparams[0]);
    fprintf(stderr, "Failed to prepare stack parameter 1: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_free_parameter_value(&stackparams[0]);
  deltacloud_free_parameter_value(&stackparams[1]);

  if (deltacloud_allocate_parameter(NULL, "foo") != NULL) {
    fprintf(stderr, "Expected deltacloud_allocate_parameter to fail with NULL name, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_allocate_parameter(NULL, NULL) != NULL) {
    fprintf(stderr, "Expected deltacloud_allocate_parameter to fail with NULL name, but succeeded\n");
    goto cleanup;
  }

  heapparams[0] = deltacloud_allocate_parameter("name", "foo");
  if (heapparams[0] == NULL) {
    fprintf(stderr, "Failed to prepare heap parameter 0: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  heapparams[1] = deltacloud_allocate_parameter("keyname", "chris");
  if (heapparams[1] == NULL) {
    deltacloud_free_parameter(heapparams[0]);
    fprintf(stderr, "Failed to prepare heap parameter 1: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_free_parameter(heapparams[0]);
  deltacloud_free_parameter(heapparams[1]);

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
