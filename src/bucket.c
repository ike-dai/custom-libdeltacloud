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

/** @file */

static void free_metadata(struct deltacloud_bucket_blob_metadata *metadata)
{
  SAFE_FREE(metadata->key);
  SAFE_FREE(metadata->value);
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

/**
 * A function to get a linked list of all of the buckets defined.  The caller
 * is expected to free the list using deltacloud_free_bucket_list().
 * @param[in] api The deltacloud_api structure representing this connection
 * @param[out] buckets A pointer to the deltacloud_bucket structure to hold
 *                     the list of buckets
 * @returns 0 on success, -1 on error
 */
int deltacloud_get_buckets(struct deltacloud_api *api,
			   struct deltacloud_bucket **buckets)
{
  return internal_get(api, "buckets", "buckets", parse_bucket_xml,
		      (void **)buckets);
}

/**
 * A function to look up a particular bucket by id.  The caller is expected
 * to free the deltacloud_bucket structure using deltacloud_free_bucket().
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] id The bucket ID to look for
 * @param[out] bucket The deltacloud_bucket structure to fill in if the ID
 *                     is found
 * @returns 0 on success, -1 if the bucket cannot be found or on error
 */
int deltacloud_get_bucket_by_id(struct deltacloud_api *api, const char *id,
				struct deltacloud_bucket *bucket)
{
  return internal_get_by_id(api, id, "buckets", "bucket", parse_one_bucket,
			    bucket);
}

/**
 * A function to create a new bucket.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] name The name to give to the new bucket
 * @param[in] params An array of deltacloud_create_parameter structures that
 *                   represent any optional parameters to pass into the
 *                   create call
 * @param[in] params_length An integer describing the length of the params
 *                          array
 * @returns 0 on success, -1 on error
 */
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

  if (internal_create(api, "buckets", internal_params, pos, NULL, NULL) < 0)
    /* internal_create already set the error */
    goto cleanup;

  ret = 0;

 cleanup:
  free_parameters(internal_params, pos);
  SAFE_FREE(internal_params);

  return ret;
}

/**
 * A function to create a new blob in a bucket.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] bucket The deltacloud_bucket representing the bucket in which
 *                   to create the blob
 * @param[in] blob_name The name to give to the new blob
 * @param[in] filename The filename from which to read the data for the blob
 * @param[in] params An array of deltacloud_create_parameter structures that
 *                   represent any optional parameters to pass into the
 *                   create call
 * @param[in] params_length An integer describing the length of the params
 *                          array
 * @returns 0 on success, -1 on error
 */
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

/**
 * A function to lookup a blob by id.  It is up to the caller to free the
 * structure with deltacloud_free_bucket_blob().
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] bucket The deltacloud_bucket representing the bucket in which
 *                   to look for the blob
 * @param[in] name The name of the blob to lookup
 * @param[out] blob The deltacloud_bucket_blob structure that will be filled
 *                  in if the blob is found
 * @returns 0 on success, -1 on error or if the blob could not be found
 */
int deltacloud_bucket_get_blob_by_id(struct deltacloud_api *api,
				     struct deltacloud_bucket *bucket,
				     const char *name,
				     struct deltacloud_bucket_blob *blob)
{
  char *bucketurl;
  int rc;

  if (!valid_api(api) || !valid_arg(bucket) || !valid_arg(name) ||
      !valid_arg(blob))
    return -1;

  if (asprintf(&bucketurl, "buckets/%s", bucket->id) < 0) {
    oom_error();
    return -1;
  }

  rc = internal_get_by_id(api, name, bucketurl, "blob", parse_one_blob,
			  blob);
  SAFE_FREE(bucketurl);

  return rc;
}

/**
 * A function to update the metadata on a blob.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] blob The deltacloud_bucket_blob structure representing the blob
 * @param[in] params An array of deltacloud_create_parameter structures that
 *                   represent the key/value pairs of metadata to update
 * @param[in] params_length An integer describing the length of the params
 *                          array
 * @returns 0 on success, -1 on error
 */
int deltacloud_bucket_blob_update_metadata(struct deltacloud_api *api,
					   struct deltacloud_bucket_blob *blob,
					   struct deltacloud_create_parameter *params,
					   int params_length)
{
  struct deltacloud_link *thislink;
  struct curl_slist *headers = NULL;
  char *bloburl;
  int ret = -1;
  int i;
  char *internal_data = NULL;
  char *header;

  if (!valid_api(api) || !valid_arg(blob))
    return -1;

  thislink = api_find_link(api, "buckets");
  if (thislink == NULL)
    /* api_find_link set the error */
    return -1;

  if (asprintf(&bloburl, "%s/%s/%s", thislink->href, blob->bucket_id,
	       blob->id) < 0) {
    oom_error();
    return -1;
  }

  /* here we have to build up the request headers we want to pass along */
  for (i = 0; i < params_length; i++) {
    if (params[i].value != NULL) {
      if (asprintf(&header, "%s: %s", params[i].name, params[i].value) < 0) {
	oom_error();
	goto cleanup;
      }

      /* you might think that we are losing the pointer to header here, and
       * hence leaking memory.  However, curl_slist takes over maintenance of
       * the memory, and it will get properly freed during curl_slist_free_all
       */
      headers = curl_slist_append(headers, header);
      if (headers == NULL) {
	oom_error();
	goto cleanup;
      }
    }
  }

  if (post_url_with_headers(bloburl, api->user, api->password, headers,
			    &internal_data) != 0)
    /* post_url_with_headers sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (internal_data != NULL && is_error_xml(internal_data)) {
    set_xml_error(internal_data, DELTACLOUD_POST_URL_ERROR);
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(internal_data);
  curl_slist_free_all(headers);
  SAFE_FREE(bloburl);

  return ret;
}

/**
 * A function to get the contents of a blob.  It is the responsibility of the
 * caller to free the memory returned in output.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] blob The deltacloud_bucket_blob structure representing the blob
 * @param[out] output A pointer to an memory location to store the blob contents
 * @returns 0 on success, -1 on error
 */
int deltacloud_bucket_blob_get_content(struct deltacloud_api *api,
				       struct deltacloud_bucket_blob *blob,
				       char **output)
{
  struct deltacloud_link *thislink;
  char *bloburl;
  int ret = -1;

  if (!valid_api(api) || !valid_arg(blob) || !valid_arg(output))
    return -1;

  thislink = api_find_link(api, "buckets");
  if (thislink == NULL)
    /* api_find_link set the error */
    return -1;

  if (asprintf(&bloburl, "%s/%s/%s/content", thislink->href, blob->bucket_id,
	       blob->id) < 0) {
    oom_error();
    return -1;
  }

  if (get_url(bloburl, api->user, api->password, output) != 0)
    /* get_url sets its own errors, so don't overwrite it here */
    return -1;

  if (*output == NULL) {
    /* if we made it here, it means that the transfer was successful (ret
     * was 0), but the data that we expected wasn't returned.  This is probably
     * a deltacloud server bug, so just set an error and bail out
     */
    data_error("buckets");
    goto cleanup;
  }

  if (is_error_xml(*output)) {
    set_xml_error(*output, DELTACLOUD_GET_URL_ERROR);
    SAFE_FREE(*output);
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(bloburl);

  return ret;
}

/**
 * A function to delete a blob.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] blob The deltacloud_bucket_blob structure representing the blob
 * @returns 0 on success, -1 on error
 */
int deltacloud_bucket_delete_blob(struct deltacloud_api *api,
				  struct deltacloud_bucket_blob *blob)
{
  if (!valid_api(api) || !valid_arg(blob))
    return -1;

  return internal_destroy(blob->href, api->user, api->password);
}

/**
 * A function to free a deltacloud_bucket_blob structure initially allocated
 * through deltacloud_bucket_get_blob_by_id().
 * @param[in] blob The deltacloud_bucket_blob structure representing the blob
 */
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
  free_list(&blob->metadata, struct deltacloud_bucket_blob_metadata,
	    free_metadata);
}

/**
 * A function to destroy a bucket.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] bucket The deltacloud_bucket structure representing the bucket
 * @returns 0 on success, -1 on error
 */
int deltacloud_bucket_destroy(struct deltacloud_api *api,
			      struct deltacloud_bucket *bucket)
{
  if (!valid_api(api) || !valid_arg(bucket))
    return -1;

  return internal_destroy(bucket->href, api->user, api->password);
}

/**
 * A function to free a deltacloud_bucket structure initially allocated
 * by deltacloud_get_bucket_by_id().
 * @param[in] bucket The deltacloud_bucket structure representing the bucket
 */
void deltacloud_free_bucket(struct deltacloud_bucket *bucket)
{
  if (bucket == NULL)
    return;

  SAFE_FREE(bucket->href);
  SAFE_FREE(bucket->id);
  SAFE_FREE(bucket->name);
  SAFE_FREE(bucket->size);
  free_list(&bucket->blobs, struct deltacloud_bucket_blob,
	    deltacloud_free_bucket_blob);
}

/**
 * A function to free a list of deltacloud_bucket structures initially allocated
 * by deltacloud_get_buckets().
 * @param[in] buckets The pointer to the head of the deltacloud_bucket list
 */
void deltacloud_free_bucket_list(struct deltacloud_bucket **buckets)
{
  free_list(buckets, struct deltacloud_bucket, deltacloud_free_bucket);
}
