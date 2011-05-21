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

#ifndef LIBDELTACLOUD_BUCKET_H
#define LIBDELTACLOUD_BUCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing a single piece of blob metadata.
 */
struct deltacloud_bucket_blob_metadata {
  char *key; /**< The key for this piece of metadata */
  char *value; /**< The value for this piece of metadata */

  struct deltacloud_bucket_blob_metadata *next;
};

/**
 * A structure representing a single blob inside of a bucket.
 */
struct deltacloud_bucket_blob {
  char *href; /**< The full URL to this blob */
  char *id; /**< The ID for this blob */
  char *bucket_id; /**< The ID of the bucket this blob is stored in */
  char *content_length; /**< The length of this blob */
  char *content_type; /**< The Content-Type of this blob */
  char *last_modified; /**< The last time this blob was modified */
  char *content_href; /**< A URL to the content for this blob */
  struct deltacloud_bucket_blob_metadata *metadata; /**< A list of all of the metadata associated with this blob */

  struct deltacloud_bucket_blob *next;
};

/**
 * A structure representing a single bucket.
 */
struct deltacloud_bucket {
  char *href; /**< The full URL to this bucket */
  char *id; /**< The ID for this bucket */
  char *name; /**< The name for this bucket */
  char *size; /**< The size of all of the objects inside of this bucket */
  struct deltacloud_bucket_blob *blobs; /**< A list of all of the blobs stored in this bucket */

  struct deltacloud_bucket *next;
};

#define deltacloud_supports_buckets(api) deltacloud_has_link(api, "buckets")
int deltacloud_get_buckets(struct deltacloud_api *api,
			   struct deltacloud_bucket **buckets);
int deltacloud_get_bucket_by_id(struct deltacloud_api *api, const char *id,
				struct deltacloud_bucket *bucket);
int deltacloud_create_bucket(struct deltacloud_api *api,
			     const char *name,
			     struct deltacloud_create_parameter *params,
			     int params_length);
int deltacloud_bucket_create_blob_from_file(struct deltacloud_api *api,
					    struct deltacloud_bucket *bucket,
					    const char *blob_name,
					    const char *filename,
					    struct deltacloud_create_parameter *params,
					    int params_length);
int deltacloud_bucket_get_blob_by_id(struct deltacloud_api *api,
				     struct deltacloud_bucket *bucket,
				     const char *name,
				     struct deltacloud_bucket_blob *blob);
int deltacloud_bucket_blob_update_metadata(struct deltacloud_api *api,
					   struct deltacloud_bucket_blob *blob,
					   struct deltacloud_create_parameter *params,
					   int params_length);
int deltacloud_bucket_blob_get_content(struct deltacloud_api *api,
				       struct deltacloud_bucket_blob *blob,
				       char **output);
int deltacloud_bucket_delete_blob(struct deltacloud_api *api,
				  struct deltacloud_bucket_blob *blob);
void deltacloud_free_bucket_blob(struct deltacloud_bucket_blob *blob);
int deltacloud_bucket_destroy(struct deltacloud_api *api,
			      struct deltacloud_bucket *bucket);
void deltacloud_free_bucket(struct deltacloud_bucket *bucket);
void deltacloud_free_bucket_list(struct deltacloud_bucket **buckets);

#ifdef __cplusplus
}
#endif

#endif
