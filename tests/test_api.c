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

static void print_api(struct deltacloud_api *api)
{
  fprintf(stderr, "API %s:\n", api->url);
  fprintf(stderr, "\tUser: %s, Driver: %s, Version: %s\n", api->user,
	  api->driver, api->version);
  print_link_list(api->links);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }
  print_api(&api);

  deltacloud_free(&api);

  return 0;
}
