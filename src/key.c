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
#include "key.h"

/** @file */

static int parse_one_key(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			 void *output)
{
  struct deltacloud_key *thiskey = (struct deltacloud_key *)output;

  memset(thiskey, 0, sizeof(struct deltacloud_key));

  thiskey->href = (char *)xmlGetProp(cur, BAD_CAST "href");
  thiskey->id = (char *)xmlGetProp(cur, BAD_CAST "id");
  thiskey->type = (char *)xmlGetProp(cur, BAD_CAST "type");
  thiskey->state = getXPathString("string(./state)", ctxt);
  thiskey->fingerprint = getXPathString("string(./fingerprint)", ctxt);

  return 0;
}

static int parse_key_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data)
{
  struct deltacloud_key **keys = (struct deltacloud_key **)data;
  struct deltacloud_key *thiskey;
  int ret = -1;
  xmlNodePtr oldnode;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "key")) {

      ctxt->node = cur;

      thiskey = calloc(1, sizeof(struct deltacloud_key));
      if (thiskey == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_key(cur, ctxt, thiskey) < 0) {
	/* parse_one_key is expected to have set its own error */
	SAFE_FREE(thiskey);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(keys, struct deltacloud_key, thiskey);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_key_list(keys);

  return ret;
}

/**
 * A function to get a linked list of all of the keys.  The caller
 * is expected to free the list using deltacloud_free_key_list().
 * @param[in] api The deltacloud_api structure representing this connection
 * @param[out] keys A pointer to the deltacloud_key structure to hold the list
 *                  of keys
 * @returns 0 on success, -1 on error
 */
int deltacloud_get_keys(struct deltacloud_api *api,
			struct deltacloud_key **keys)
{
  return internal_get(api, "keys", "keys", parse_key_xml, (void **)keys);
}

/**
 * A function to create a new key.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] name The name to give to the new key
 * @param[in] params An array of deltacloud_create_parameter structures that
 *                   represent any optional parameters to pass into the
 *                   create call
 * @param[in] params_length An integer describing the length of the params
 *                          array
 * @returns 0 on success, -1 on error
 */
int deltacloud_create_key(struct deltacloud_api *api, const char *name,
			  struct deltacloud_create_parameter *params,
			  int params_length)
{
  struct deltacloud_create_parameter *internal_params;
  int ret = -1;
  int pos;

  if (!valid_api(api) || !valid_arg(name))
    return -1;

  if (params_length < 0) {
    invalid_argument_error("params_length must be >= 0");
    return -1;
  }

  internal_params = calloc(params_length + 1,
			   sizeof(struct deltacloud_create_parameter));
  if (internal_params == NULL) {
    oom_error();
    return -1;
  }

  pos = copy_parameters(internal_params, params, params_length);
  if (pos < 0)
    /* copy_parameters already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "name", name) < 0)
    /* deltacloud_create_parameter already set the error */
    goto cleanup;

  if (internal_create(api, "keys", internal_params, pos, NULL, NULL) < 0)
    /* internal_create already set the error */
    goto cleanup;

  ret = 0;

 cleanup:
  free_parameters(internal_params, pos);
  SAFE_FREE(internal_params);

  return ret;
}

/**
 * A function to destroy a key.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] key The deltacloud_key structure representing the key
 * @returns 0 on success, -1 on error
 */
int deltacloud_key_destroy(struct deltacloud_api *api,
			   struct deltacloud_key *key)
{
  if (!valid_api(api) || !valid_arg(key))
    return -1;

  return internal_destroy(key->href, api->user, api->password, api->driver, api->provider);
}

/**
 * A function to look up a particular key by id.  The caller is expected
 * to free the deltacloud_key structure using deltacloud_free_key().
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] id The key ID to look for
 * @param[out] key The deltacloud_key structure to fill in if the ID is found
 * @returns 0 on success, -1 if the key cannot be found or on error
 */
int deltacloud_get_key_by_id(struct deltacloud_api *api, const char *id,
			     struct deltacloud_key *key)
{
  return internal_get_by_id(api, id, "keys", "key", parse_one_key, key);
}

/**
 * A function to free a deltacloud_key structure initially allocated
 * by deltacloud_get_key_by_id().
 * @param[in] key The deltacloud_key structure representing the key
 */
void deltacloud_free_key(struct deltacloud_key *key)
{
  if (key == NULL)
    return;

  SAFE_FREE(key->href);
  SAFE_FREE(key->id);
  SAFE_FREE(key->type);
  SAFE_FREE(key->state);
  SAFE_FREE(key->fingerprint);
}

/**
 * A function to free a list of deltacloud_key structures initially
 * allocated by deltacloud_get_keys().
 * @param[in] keys The pointer to the head of the deltacloud_key list
 */
void deltacloud_free_key_list(struct deltacloud_key **keys)
{
  free_list(keys, struct deltacloud_key, deltacloud_free_key);
}

