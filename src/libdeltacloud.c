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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <pthread.h>
#include "libdeltacloud.h"
#include "common.h"
#include "curl_action.h"

/** @file */

/** \mainpage libdeltacloud main page
 * \section Introduction
 * libdeltacloud is a library for accessing the deltacloud API through a
 * convenient C library.
 * \section api_abi_stability API and ABI stability
 * As of version 0.9, libdeltacloud is mostly API, but not ABI, stable.  The
 * difference between the two is subtle but important.  A library that is ABI
 * (Application Binary Interface) stable means that programs that use the
 * library will not need to be modified nor re-compiled when a new version of
 * the library comes out.  A library that is API (Application Programming
 * Interface) stable means that programs that use the library will not need
 * to be modified, but may need to be re-compiled when a new version of the
 * library comes out.  That is because the sizes of structures in the library
 * might change, and a mis-match between what the library thinks the size of
 * a structure is and what the program thinks the size of a structure is is
 * a recipe for disaster.
 *
 * Due to the magic of libtool versioning, programs built against an older
 * version of libdeltacloud will refuse to run against a newer version of
 * libdeltacloud if the size of the structures has changed.  If this happens,
 * then the program must be recompiled against the newer libdeltacloud.
 * \section examples Examples
 * \subsection example1 Connect to deltacloud
 * \code
 * #include <stdio.h>
 * #include <stdlib.h>
 * #include <libdeltacloud/libdeltacloud.h>
 *
 * int main()
 * {
 *   struct deltacloud_api api;
 *
 *   if (deltacloud_initialize(&api, "http://localhost:3001/api", "mockuser",
 *                             "mockpassword") < 0) {
 *       fprintf(stderr, "Failed to initialize libdeltacloud: %s\n",
 *               deltacloud_get_last_error_string());
 *       return 1;
 *   }
 *
 *   deltacloud_free(&api);
 *   return 0;
 * }
 * \endcode
 * \subsection example2 List all running instances
 * \code
 * #include <stdio.h>
 * #include <stdlib.h>
 * #include <libdeltacloud/libdeltacloud.h>
 *
 * int main()
 * {
 *   struct deltacloud_api api;
 *   struct deltacloud_instances *instances = NULL;
 *   int ret = 2;
 *
 *   if (deltacloud_initialize(&api, "http://localhost:3001/api", "mockuser",
 *                             "mockpassword") < 0) {
 *       fprintf(stderr, "Failed to initialize libdeltacloud: %s\n",
 *               deltacloud_get_last_error_string());
 *       return 1;
 *   }
 *
 *   if (deltacloud_get_instances(&api, &instances) < 0) {
 *       fprintf(stderr, "Failed to get deltacloud instances: %s\n",
 *               deltacloud_get_last_error_string());
 *       goto cleanup;
 *   }
 *
 *   deltacloud_free_instance_list(&instances);
 *
 *   ret = 0;
 *
 *  cleanup:
 *   deltacloud_free(&api);
 *   return ret;
 * }
 * \endcode
 */

/* On first initialization of the library we setup a per-thread local
 * variable to hold errors.  If one of the API calls subsequently fails,
 * then we set the per-thread variable with details of the failure.
 */
static int tlsinitialized = 0;

/**
 * A function to get a pointer to the deltacloud_error structure corresponding
 * to the last failure.  The results of this are undefined if no error occurred.
 * As the pointer is to thread-local storage that libdeltacloud manages, the
 * caller should \b not attempt to free the structure.
 * @returns A deltacloud_error structure corresponding to the last failure
 */
struct deltacloud_error *deltacloud_get_last_error(void)
{
  return pthread_getspecific(deltacloud_last_error);
}

/**
 * A function to get the details (as a string) corresponding to the last
 * failure.  The results of this are undefined if no error occurred.  As the
 * string is to thread-local storage that libdeltacloud manages, the caller
 * should \b not attempt to free the string.
 * @returns A string description of the last failure
 */
const char *deltacloud_get_last_error_string(void)
{
  struct deltacloud_error *last;

  last = deltacloud_get_last_error();
  if (last != NULL)
    return last->details;
  return NULL;
}

/** @cond INTERNAL
 *
 * instead of putting this in a header we replicate it here so the header
 * files don't need to include libxml2 headers.  This saves client programs
 * from having to have -I/path/to/libxml2/headers in their build paths.
 */
int parse_link_xml(xmlNodePtr linknode, struct deltacloud_link **links);
/** @endcond */

static int parse_api_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void *data)
{
  struct deltacloud_api *api = (struct deltacloud_api *)data;
  int ret = -1;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "api")) {
      api->driver = (char *)xmlGetProp(cur, BAD_CAST "driver");
      api->version = (char *)xmlGetProp(cur, BAD_CAST "version");

      if (parse_link_xml(cur->children, &(api->links)) < 0)
	goto cleanup;
    }

    cur = cur->next;
  }

  ret = 0;

 cleanup:
  /* if we hit some kind of failure here, in theory we would want to clean
   * up the api structure.  However, deltacloud_initialize() also does some
   * allocation of memory, so it does a deltacloud_free() on error, so we leave
   * it up to deltacloud_initialize() to cleanup.
   */

  return ret;
}

static void internal_free(struct deltacloud_api *api)
{
  free_link_list(&api->links);
  SAFE_FREE(api->user);
  SAFE_FREE(api->password);
  SAFE_FREE(api->url);
  SAFE_FREE(api->driver);
  SAFE_FREE(api->version);
  memset(api, 0, sizeof(struct deltacloud_api));
}

/**
 * The main API entry point.  All users of the library \b must call this
 * function first to initialize the library.  The caller must free the
 * deltacloud_api structure using deltacloud_free() when finished.
 * @param[in,out] api The api structure
 * @param[in] url The url to the deltacloud server
 * @param[in] user The username required to connect to the deltacloud server
 * @param[in] password The password required to connect to the deltacloud server
 * @returns 0 on success, -1 on error
 */
int deltacloud_initialize(struct deltacloud_api *api, char *url, char *user,
			  char *password)
{
  char *data = NULL;
  int ret = -1;

  if (!tlsinitialized) {
    tlsinitialized = 1;
    if (pthread_key_create(&deltacloud_last_error,
			   deltacloud_error_free_data) != 0) {
      dcloudprintf("Failed to initialize error handler\n");
      return -1;
    }
  }

  if (!valid_arg(api) || !valid_arg(url) || !valid_arg(user) ||
      !valid_arg(password))
    return -1;

  memset(api, 0, sizeof(struct deltacloud_api));

  api->url = strdup(url);
  if (api->url == NULL) {
    oom_error();
    return -1;
  }
  api->user = strdup(user);
  if (api->user == NULL) {
    oom_error();
    goto cleanup;
  }
  api->password = strdup(password);
  if (api->password == NULL) {
    oom_error();
    goto cleanup;
  }

  if (get_url(api->url, api->user, api->password, &data) != 0)
    /* get_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (data == NULL) {
    /* if we made it here, it means that the transfer was successful (ret
     * was 0), but the data that we expected wasn't returned.  This is probably
     * a deltacloud server bug, so just set an error and bail out
     */
    set_error(DELTACLOUD_GET_URL_ERROR, "Expected link data, received nothing");
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml_single(data, "api", parse_api_xml, api) < 0)
    goto cleanup;

  api->initialized = 0xfeedbeef;

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  if (ret < 0)
    internal_free(api);
  return ret;
}

/**
 * A function to prepare a deltacloud_create_parameter structure for use.  A
 * deltacloud_create_parameter structure is used as an optional input parameter
 * to one of the deltacloud_create functions.  It is up to the caller to free
 * the memory allocated to the structure with deltacloud_free_parameter_value().
 * @param[in,out] param The deltacloud_create_parameter structure to prepare
 *                      for use
 * @param[in] name The name to assign to the deltacloud_create_parameter
 *                 structure
 * @param[in] value The value to assign to the deltacloud_create_parameter
 *                  structure
 * @returns 0 on success, -1 on error
 */
int deltacloud_prepare_parameter(struct deltacloud_create_parameter *param,
				 const char *name, const char *value)
{
  param->name = strdup(name);
  if (param->name == NULL) {
    oom_error();
    return -1;
  }

  param->value = strdup(value);
  if (param->value == NULL) {
    SAFE_FREE(param->name);
    oom_error();
    return -1;
  }

  return 0;
}

/**
 * A function to allocate and prepare a deltacloud_create_parameter structure
 * for use.  A deltacloud_create_parameter structure is used as an optional
 * input parameter to one of the deltacloud_create functions.  It is up to the
 * caller to free the structure with deltacloud_free_parameter().
 * @param[in] name The name to assign to the deltacloud_create_parameter
 *                 structure
 * @param[in] value The value to assign to the deltacloud_create_parameter
 *                  structure
 * @returns A newly allocated deltacloud_create_parameter structure on success,
 *          NULL on error
 */
struct deltacloud_create_parameter *deltacloud_allocate_parameter(const char *name,
								  const char *value)
{
  struct deltacloud_create_parameter *ret;

  ret = malloc(sizeof(struct deltacloud_create_parameter));
  if (ret == NULL) {
    oom_error();
    return NULL;
  }

  if (deltacloud_prepare_parameter(ret, name, value) < 0) {
    /* deltacloud_prepare_parameter set the error already */
    SAFE_FREE(ret);
    return NULL;
  }

  return ret;
}

/**
 * A function to free the memory assigned to a deltacloud_create_parameter
 * structure (typically by deltacloud_prepare_parameter()).
 * @param[in] param The deltacloud_create_parameter structure to free memory
 *                  from
 */
void deltacloud_free_parameter_value(struct deltacloud_create_parameter *param)
{
  SAFE_FREE(param->name);
  SAFE_FREE(param->value);
}

/**
 * A function to free a deltacloud_create_parameter structure (typically
 * allocated by deltacloud_allocate_parameter()).
 * @param[in] param The deltacloud_create_parameter to free
 */
void deltacloud_free_parameter(struct deltacloud_create_parameter *param)
{
  deltacloud_free_parameter_value(param);
  SAFE_FREE(param);
}

/**
 * A function to determine if the deltacloud_api structure contains a particular
 * link.  This can be used to determine if the deltacloud server on the other
 * end supports certain features.
 * @param[in] api The deltacloud_api structure representing this connection
 * @param[in] name The feature to find
 * @returns 1 if the feature is supported, 0 if the feature is not supported,
 *          and -1 on error
 */
int deltacloud_has_link(struct deltacloud_api *api, const char *name)
{
  struct deltacloud_link *link;

  if (!valid_api(api) || !valid_arg(name))
    return -1;

  deltacloud_for_each(link, api->links) {
    if (strcmp(link->rel, name) == 0)
      return 1;
  }

  return 0;
}

/**
 * A function to free up a deltacloud_api structure originally configured
 * through deltacloud_initialize().
 * @param[in] api The deltacloud_api structure to free
 */
void deltacloud_free(struct deltacloud_api *api)
{
  if (!valid_api(api))
    return;

  internal_free(api);
}
