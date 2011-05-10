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
#include <unistd.h>
#include "libdeltacloud.h"
#include "test_common.h"

static void print_bucket(struct deltacloud_bucket *bucket)
{
  struct deltacloud_bucket_blob *blob;

  fprintf(stderr, "Bucket: %s\n", bucket->id);
  fprintf(stderr, "\tHref: %s\n", bucket->href);
  fprintf(stderr, "\tName: %s\n", bucket->name);
  fprintf(stderr, "\tSize: %s\n", bucket->size);

  deltacloud_for_each(blob, bucket->blobs) {
    fprintf(stderr, "\tBlob: %s\n", blob->id);
    fprintf(stderr, "\t\tHref: %s\n", blob->href);
  }
}

static void print_bucket_list(struct deltacloud_bucket *buckets)
{
  struct deltacloud_bucket *bucket;

  deltacloud_for_each(bucket, buckets)
    print_bucket(bucket);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_api zeroapi;
  struct deltacloud_bucket bucket;
  struct deltacloud_bucket *buckets = NULL;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  memset(&zeroapi, 0, sizeof(struct deltacloud_api));

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }

  /* test out deltacloud_supports_buckets */
  if (deltacloud_supports_buckets(NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_buckets to fail with NULL api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_buckets(&zeroapi) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_buckets to fail with uninitialized api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_buckets(&api)) {

    /* test out deltacloud_get_buckets */
    if (deltacloud_get_buckets(NULL, &buckets) >= 0) {
      fprintf(stderr, "Expected deltacloud_supports_buckets to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_buckets(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_buckets to fail with NULL buckets, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_buckets(&zeroapi, &buckets) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_buckets to fail with unintialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_buckets(&api, &buckets) < 0) {
      fprintf(stderr, "Failed to get buckets: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_bucket_list(buckets);

    if (buckets != NULL) {

      /* test out deltacloud_get_bucket_by_id */
      if (deltacloud_get_bucket_by_id(NULL, buckets->id, &bucket) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_bucket_by_id to fail with NULL api, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_bucket_by_id(&api, NULL, &bucket) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_bucket_by_id to fail with NULL id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_bucket_by_id(&api, buckets->id, NULL) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_bucket_by_id to fail with NULL bucket, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_bucket_by_id(&api, "bogus_id", &bucket) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_bucket_by_id to fail with bogus id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_bucket_by_id(&zeroapi, buckets->id, &bucket) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_bucket_by_id to fail with unintialized api, but succeeded\n");
	goto cleanup;
      }

      /* here we use the first bucket from the list above */
      if (deltacloud_get_bucket_by_id(&api, buckets->id, &bucket) < 0) {
	fprintf(stderr, "Failed to get bucket by id: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }
      print_bucket(&bucket);
      deltacloud_free_bucket(&bucket);
    }
  }

 cleanup:
  deltacloud_free_bucket_list(&buckets);

  deltacloud_free(&api);

  return ret;
}
