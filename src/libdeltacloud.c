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

/* On first initialization of the library we setup a per-thread local
 * variable to hold errors.  If one of the API calls subsequently fails,
 * then we set the per-thread variable with details of the failure.
 */
static int tlsinitialized = 0;

struct deltacloud_error *deltacloud_get_last_error(void)
{
  return pthread_getspecific(deltacloud_last_error);
}

const char *deltacloud_get_last_error_string(void)
{
  struct deltacloud_error *last;

  last = deltacloud_get_last_error();
  if (last != NULL)
    return last->details;
  return NULL;
}

/* instead of putting this in a header we replicate it here so the header
 * files don't need to include libxml2 headers.  This saves client programs
 * from having to have -I/path/to/libxml2/headers in their build paths.
 */
int parse_link_xml(xmlNodePtr linknode, struct deltacloud_link **links);

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

void deltacloud_free_parameter_value(struct deltacloud_create_parameter *param)
{
  SAFE_FREE(param->name);
  SAFE_FREE(param->value);
}

void deltacloud_free_parameter(struct deltacloud_create_parameter *param)
{
  deltacloud_free_parameter_value(param);
  SAFE_FREE(param);
}

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

void deltacloud_free(struct deltacloud_api *api)
{
  if (!valid_api(api))
    return;

  internal_free(api);
}
