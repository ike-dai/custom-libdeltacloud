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

static void print_hwp_list(struct deltacloud_hardware_profile *profiles)
{
  struct deltacloud_hardware_profile *hwp;

  deltacloud_for_each(hwp, profiles)
    print_hwp(hwp);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_api zeroapi;
  struct deltacloud_hardware_profile profile;
  struct deltacloud_hardware_profile *profiles = NULL;
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

  /* test out deltacloud_supports_hardware_profiles */
  if (deltacloud_supports_hardware_profiles(NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_hardware_profiles to fail with NULL api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_hardware_profiles(&zeroapi) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_hardware_profiles to fail with uninitialized api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_hardware_profiles(&api)) {

    /* test out deltacloud_get_hardware_profiles */
    if (deltacloud_get_hardware_profiles(NULL, &profiles) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_hardware_profiles to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_hardware_profiles(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_hardware_profiles to fail with NULL hardware_profiles, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_hardware_profiles(&zeroapi, &profiles) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_hardware_profiles to fail with unintialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_hardware_profiles(&api, &profiles) < 0) {
      fprintf(stderr, "Failed to get hardware_profiles: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_hwp_list(profiles);

    if (profiles != NULL) {

      /* test out deltacloud_get_hardware_profile_by_id */
      if (deltacloud_get_hardware_profile_by_id(NULL, profiles->id,
						&profile) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_hardware_profile_by_id to fail with NULL api, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_hardware_profile_by_id(&api, NULL, &profile) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_hardware_profile_by_id to fail with NULL id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_hardware_profile_by_id(&api, profiles->id,
						NULL) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_hardware_profile_by_id to fail with NULL hardware_profile, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_hardware_profile_by_id(&api, "bogus_id",
						&profile) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_hardware_profile_by_id to fail with bogus id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_hardware_profile_by_id(&zeroapi, profiles->id,
						&profile) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_hardware_profile_by_id to fail with unintialized api, but succeeded\n");
	goto cleanup;
      }

      /* here we use the first hardware profile from the list above */
      if (deltacloud_get_hardware_profile_by_id(&api, profiles->id,
						&profile) < 0) {
	fprintf(stderr, "Failed to get hardware profile by id: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }
      print_hwp(&profile);
      deltacloud_free_hardware_profile(&profile);
    }
  }
  else
    fprintf(stderr, "Hardware Profiles are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free_hardware_profile_list(&profiles);
  deltacloud_free(&api);

  return ret;
}
