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

struct deltacloud_bucket_blob_metadata {
  char *key;
  char *value;

  struct deltacloud_bucket_blob_metadata *next;
};

struct deltacloud_bucket_blob {
  char *href;
  char *id;
  char *bucket_id;
  char *content_length;
  char *content_type;
  char *last_modified;
  char *content_href;
  struct deltacloud_bucket_blob_metadata *metadata;

  struct deltacloud_bucket_blob *next;
};

struct deltacloud_bucket {
  char *href;
  char *id;
  char *name;
  char *size;
  struct deltacloud_bucket_blob *blobs;

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
