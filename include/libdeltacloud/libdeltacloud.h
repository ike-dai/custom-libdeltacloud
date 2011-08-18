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

/**
 * The main libdeltacloud structure.  Since almost all libdeltacloud calls
 * require a deltacloud_api structure, this structure must be initialized first
 * via a call to deltacloud_initialize().
 */
struct deltacloud_api {
  char *url; /**< The top-level URL to the deltacloud server */
  char *user; /**< The username used to connect to the deltacloud server */
  char *password; /**< The password used to connect to the deltacloud server */
  char *driver; /**< The driver in use with the connection (this is determined automatically from the URL) */
  char *version; /**< The version of the deltacloud API as returned by the server */

  int initialized; /**< An internal field used to determine if the deltacloud_api structure has been properly initialized */

  struct deltacloud_link *links; /**< A list of links pointing to the various components (instances, images, etc) available */
};

/**
 * A structure representing a libdeltacloud error.  Errors are stored in
 * thread-safe variables.  A pointer to the error structure for this thread
 * can be obtained by using deltacloud_get_last_error().
 */
struct deltacloud_error {
  int error_num; /**< The error number associated with an error, one of the DELTACLOUD* errors */
  char *details; /**< A string representing details of the error */
};

/**
 * A structure to represent additional URL parameters to pass to various
 * libdeltacloud calls.  For instance, if the name field was set to "foo" and
 * the value field was set to "bar", then the URI used to access a resource
 * would have the string "foo=bar" appended.
 */
struct deltacloud_create_parameter {
  char *name; /**< The name to use for this parameter */
  char *value; /**< The value to use for this parameter */
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
#include "firewall.h"

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
#define DELTACLOUD_MULTIPART_POST_URL_ERROR -13
#define DELTACLOUD_INTERNAL_ERROR -14

#ifdef __cplusplus
}
#endif

#endif
