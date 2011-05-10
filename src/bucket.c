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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "curl_action.h"
#include "bucket.h"

static void free_metadata(struct deltacloud_bucket_blob_metadata *metadata)
{
  SAFE_FREE(metadata->key);
  SAFE_FREE(metadata->value);
}

static void free_metadata_list(struct deltacloud_bucket_blob_metadata **metadata)
{
  free_list(metadata, struct deltacloud_bucket_blob_metadata, free_metadata);
}

static int parse_one_blob(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			  void *output)
{
  struct deltacloud_bucket_blob *thisblob = (struct deltacloud_bucket_blob *)output;
  struct deltacloud_bucket_blob_metadata *thisentry;
  xmlNodePtr meta_cur, entry_cur, oldnode;
  int ret = -1;

  oldnode = ctxt->node;

  memset(thisblob, 0, sizeof(struct deltacloud_bucket_blob));

  thisblob->href = getXPathString("string(./@href)", ctxt);
  thisblob->id = getXPathString("string(./@id)", ctxt);
  thisblob->bucket_id = getXPathString("string(./bucket)", ctxt);
  thisblob->content_length = getXPathString("string(./content_length)", ctxt);
  thisblob->content_type = getXPathString("string(./content_type)", ctxt);
  thisblob->last_modified = getXPathString("string(./last_modified)", ctxt);
  thisblob->content_href = getXPathString("string(./content/@href)", ctxt);

  meta_cur = cur->children;
  while (meta_cur != NULL) {
    if (meta_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)meta_cur->name, "user_metadata")) {

      entry_cur = meta_cur->children;
      while (entry_cur != NULL) {
	if (entry_cur->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)entry_cur->name, "entry")) {

	  ctxt->node = entry_cur;

	  thisentry = calloc(1, sizeof(struct deltacloud_bucket_blob_metadata));
	  if (thisentry == NULL) {
	    deltacloud_free_bucket_blob(thisblob);
	    oom_error();
	    goto cleanup;
	  }

	  thisentry->key = getXPathString("string(./@key)", ctxt);
	  thisentry->value = getXPathString("string(.)", ctxt);

	  if (thisentry->value != NULL) {
	    strip_trailing_whitespace(thisentry->value);
	    strip_leading_whitespace(thisentry->value);
	  }

	  /* add_to_list can't fail */
	  add_to_list(&thisblob->metadata,
		      struct deltacloud_bucket_blob_metadata, thisentry);
	}

	entry_cur = entry_cur->next;
      }
    }

    meta_cur = meta_cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;

  return ret;
}

static int parse_one_bucket(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			    void *output)
{
  struct deltacloud_bucket *thisbucket = (struct deltacloud_bucket *)output;
  struct deltacloud_bucket_blob *thisblob;
  xmlNodePtr blob_cur;
  int ret = -1;

  memset(thisbucket, 0, sizeof(struct deltacloud_bucket));

  thisbucket->href = getXPathString("string(./@href)", ctxt);
  thisbucket->id = getXPathString("string(./@id)", ctxt);
  thisbucket->name = getXPathString("string(./name)", ctxt);
  thisbucket->size = getXPathString("string(./size)", ctxt);

  blob_cur = cur->children;
  while (blob_cur != NULL) {
    if (blob_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)blob_cur->name, "blob")) {

      thisblob = calloc(1, sizeof(struct deltacloud_bucket_blob));
      if (thisblob == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_blob(blob_cur, ctxt, thisblob) < 0) {
	/* parse_one_blob already set the error */
	SAFE_FREE(thisblob);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(&thisbucket->blobs, struct deltacloud_bucket_blob, thisblob);
    }

    blob_cur = blob_cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_bucket(thisbucket);

  return ret;
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

int deltacloud_bucket_create_blob_from_file(struct deltacloud_api *api,
					    struct deltacloud_bucket *bucket,
					    const char *blob_name,
					    const char *filename,
					    struct deltacloud_create_parameter *params,
					    int params_length)
{
  struct deltacloud_link *thislink;
  int ret = -1;
  char *href = NULL;
  char *meta = NULL;
  struct curl_httppost *httppost = NULL;
  struct curl_httppost *last = NULL;
  int i;
  char *data = NULL;
  CURLcode res;

  if (!valid_api(api) || !valid_arg(bucket) || !valid_arg(blob_name) ||
      !valid_arg(filename))
    return -1;

  if (params_length < 0) {
    invalid_argument_error("params_length must be >= 0");
    return -1;
  }

  thislink = api_find_link(api, "buckets");
  if (thislink == NULL)
    /* api_find_link set the error */
    return -1;

  res = curl_formadd(&httppost, &last, CURLFORM_COPYNAME, "blob_data",
		     CURLFORM_FILE, filename, CURLFORM_END);
  if (res != CURLE_OK) {
    set_curl_error(DELTACLOUD_OOM_ERROR, "Failed to set blob_data", res);
    return -1;
  }

  res = curl_formadd(&httppost, &last, CURLFORM_COPYNAME, "blob_id",
	       CURLFORM_COPYCONTENTS, blob_name, CURLFORM_END);
  if (res != CURLE_OK) {
    set_curl_error(DELTACLOUD_OOM_ERROR, "Failed to set blob_id", res);
    goto cleanup;
  }

  if (asprintf(&meta, "%d", params_length) < 0) {
    oom_error();
    goto cleanup;
  }
  res = curl_formadd(&httppost, &last, CURLFORM_COPYNAME, "meta_params",
		     CURLFORM_COPYCONTENTS, meta, CURLFORM_END);
  SAFE_FREE(meta);
  if (res != CURLE_OK) {
    set_curl_error(DELTACLOUD_OOM_ERROR, "Failed to set metadata parameters",
		   res);
    goto cleanup;
  }

  for (i = 0; i < params_length; i++) {
    if (asprintf(&meta, "meta_name%d", i + 1) < 0) {
      oom_error();
      goto cleanup;
    }
    res = curl_formadd(&httppost, &last, CURLFORM_COPYNAME, meta,
		       CURLFORM_COPYCONTENTS, params[i].name, CURLFORM_END);
    SAFE_FREE(meta);
    if (res != CURLE_OK) {
      set_curl_error(DELTACLOUD_OOM_ERROR, "Failed to set metadata name",
		     res);
      goto cleanup;
    }

    if (asprintf(&meta, "meta_value%d", i + 1) < 0) {
      oom_error();
      goto cleanup;
    }
    res = curl_formadd(&httppost, &last, CURLFORM_COPYNAME, meta,
		 CURLFORM_COPYCONTENTS, params[i].value, CURLFORM_END);
    SAFE_FREE(meta);
    if (res != CURLE_OK) {
      set_curl_error(DELTACLOUD_OOM_ERROR, "Failed to set metadata value",
		     res);
      goto cleanup;
    }
  }

  if (asprintf(&href, "%s/%s", thislink->href, bucket->id) < 0) {
    oom_error();
    goto cleanup;
  }

  if (do_multipart_post_url(href, api->user, api->password, httppost,
			    &data) < 0)
    /* do_multipart_post_url already set the error */
    goto cleanup;

  if (data != NULL && is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_DELETE_URL_ERROR);
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  curl_formfree(httppost);
  SAFE_FREE(href);

  return ret;
}

int deltacloud_bucket_get_blob_by_id(struct deltacloud_api *api,
				     struct deltacloud_bucket *bucket,
				     const char *name,
				     struct deltacloud_bucket_blob *blob)
{
  char *bucketurl;
  int rc;

  if (asprintf(&bucketurl, "buckets/%s", bucket->id) < 0) {
    oom_error();
    return -1;
  }

  rc = internal_get_by_id(api, name, bucketurl, "blob", parse_one_blob,
			  blob);
  SAFE_FREE(bucketurl);

  return rc;
}

int deltacloud_bucket_delete_blob(struct deltacloud_api *api,
				  struct deltacloud_bucket_blob *blob)
{
  if (!valid_api(api) || !valid_arg(blob))
    return -1;

  return internal_destroy(blob->href, api->user, api->password);
}

void deltacloud_free_bucket_blob(struct deltacloud_bucket_blob *blob)
{
  if (blob == NULL)
    return;

  SAFE_FREE(blob->href);
  SAFE_FREE(blob->id);
  SAFE_FREE(blob->bucket_id);
  SAFE_FREE(blob->content_length);
  SAFE_FREE(blob->content_type);
  SAFE_FREE(blob->last_modified);
  SAFE_FREE(blob->content_href);
  free_metadata_list(&blob->metadata);
}

static void free_blob_list(struct deltacloud_bucket_blob **blobs)
{
  free_list(blobs, struct deltacloud_bucket_blob, deltacloud_free_bucket_blob);
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
