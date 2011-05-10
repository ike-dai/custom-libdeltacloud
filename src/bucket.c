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
#include "common.h"
#include "bucket.h"

static void free_blob(struct deltacloud_bucket_blob *blob)
{
  SAFE_FREE(blob->href);
  SAFE_FREE(blob->id);
}

static void free_blob_list(struct deltacloud_bucket_blob **blobs)
{
  free_list(blobs, struct deltacloud_bucket_blob, free_blob);
}

static int parse_one_bucket(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			    void *output)
{
  struct deltacloud_bucket *thisbucket = (struct deltacloud_bucket *)output;
  struct deltacloud_bucket_blob *thisblob;
  xmlNodePtr blob_cur;

  memset(thisbucket, 0, sizeof(struct deltacloud_bucket));

  thisbucket->href = (char *)xmlGetProp(cur, BAD_CAST "href");
  thisbucket->id = (char *)xmlGetProp(cur, BAD_CAST "id");
  thisbucket->name = getXPathString("string(./name)", ctxt);
  thisbucket->size = getXPathString("string(./size)", ctxt);

  blob_cur = cur->children;
  while (blob_cur != NULL) {
    if (blob_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)blob_cur->name, "blob")) {

      thisblob = calloc(1, sizeof(struct deltacloud_bucket_blob));
      if (thisblob == NULL) {
	deltacloud_free_bucket(thisbucket);
	return -1;
      }

      thisblob->href = (char *)xmlGetProp(blob_cur, BAD_CAST "href");
      thisblob->id = (char *)xmlGetProp(blob_cur, BAD_CAST "id");

      /* add_to_list can't fail */
      add_to_list(&thisbucket->blobs, struct deltacloud_bucket_blob, thisblob);
    }
    blob_cur = blob_cur->next;
  }

  return 0;
}

static int parse_bucket_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			    void **data)
{
  struct deltacloud_bucket **buckets = (struct deltacloud_bucket **)data;
  struct deltacloud_bucket *thisbucket;
  xmlNodePtr oldnode;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "bucket")) {

      ctxt->node = cur;

      thisbucket = calloc(1, sizeof(struct deltacloud_bucket));
      if (thisbucket == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_bucket(cur, ctxt, thisbucket) < 0) {
	/* parse_one_bucket is expected to have set its own error */
	SAFE_FREE(thisbucket);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(buckets, struct deltacloud_bucket, thisbucket);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_bucket_list(buckets);

  return ret;
}

int deltacloud_get_buckets(struct deltacloud_api *api,
			   struct deltacloud_bucket **buckets)
{
  return internal_get(api, "buckets", "buckets", parse_bucket_xml,
		      (void **)buckets);
}

int deltacloud_get_bucket_by_id(struct deltacloud_api *api, const char *id,
				struct deltacloud_bucket *bucket)
{
  return internal_get_by_id(api, id, "buckets", "bucket", parse_one_bucket,
			    bucket);
}

int deltacloud_create_bucket(struct deltacloud_api *api,
			     const char *name,
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
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (internal_create(api, "buckets", internal_params, pos, NULL) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  free_parameters(internal_params, pos);
  SAFE_FREE(internal_params);

  return ret;
}

int deltacloud_bucket_destroy(struct deltacloud_api *api,
			      struct deltacloud_bucket *bucket)
{
  if (!valid_api(api) || !valid_arg(bucket))
    return -1;

  return internal_destroy(bucket->href, api->user, api->password);
}

void deltacloud_free_bucket(struct deltacloud_bucket *bucket)
{
  if (bucket == NULL)
    return;

  SAFE_FREE(bucket->href);
  SAFE_FREE(bucket->id);
  SAFE_FREE(bucket->name);
  SAFE_FREE(bucket->size);
  free_blob_list(&bucket->blobs);
}

void deltacloud_free_bucket_list(struct deltacloud_bucket **buckets)
{
  free_list(buckets, struct deltacloud_bucket, deltacloud_free_bucket);
}
