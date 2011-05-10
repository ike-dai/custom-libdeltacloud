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

#ifndef LIBDELTACLOUD_H
#define LIBDELTACLOUD_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_api {
  char *url;
  char *user;
  char *password;
  char *driver;
  char *version;

  int initialized;

  struct deltacloud_link *links;
};

struct deltacloud_error {
  int error_num;
  char *details;
};

struct deltacloud_create_parameter {
  char *name;
  char *value;
};

#include "link.h"
#include "instance.h"
#include "realm.h"
#include "image.h"
#include "instance_state.h"
#include "storage_volume.h"
#include "storage_snapshot.h"
#include "hardware_profile.h"
#include "key.h"
#include "driver.h"
#include "loadbalancer.h"
#include "bucket.h"

int deltacloud_initialize(struct deltacloud_api *api, char *url, char *user,
			  char *password);

int deltacloud_prepare_parameter(struct deltacloud_create_parameter *param,
				 const char *name, const char *value);
struct deltacloud_create_parameter *deltacloud_allocate_parameter(const char *name,
								  const char *value);
void deltacloud_free_parameter_value(struct deltacloud_create_parameter *param);
void deltacloud_free_parameter(struct deltacloud_create_parameter *param);

struct deltacloud_error *deltacloud_get_last_error(void);
const char *deltacloud_get_last_error_string(void);

int deltacloud_has_link(struct deltacloud_api *api, const char *name);

void deltacloud_free(struct deltacloud_api *api);

#define deltacloud_for_each(curr, list) for (curr = list; curr != NULL; curr = curr->next)

/* Error codes */
#define DELTACLOUD_UNKNOWN_ERROR -1
/* ERROR codes -2, -3, and -4 are reserved for future use */
#define DELTACLOUD_GET_URL_ERROR -5
#define DELTACLOUD_POST_URL_ERROR -6
#define DELTACLOUD_XML_ERROR -7
#define DELTACLOUD_URL_DOES_NOT_EXIST_ERROR -8
#define DELTACLOUD_OOM_ERROR -9
#define DELTACLOUD_INVALID_ARGUMENT_ERROR -10
#define DELTACLOUD_NAME_NOT_FOUND_ERROR -11
#define DELTACLOUD_DELETE_URL_ERROR -12

#ifdef __cplusplus
}
#endif

#endif
