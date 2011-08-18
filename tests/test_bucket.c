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
#include <string.h>
#include <errno.h>
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
  struct deltacloud_bucket_blob blob;
  FILE *fp;
  struct deltacloud_create_parameter stackparams[2];
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
      fprintf(stderr, "Expected deltacloud_get_buckets to fail with NULL api, but succeeded\n");
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

    if (deltacloud_create_bucket(NULL, "aname", NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_bucket to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_bucket(&api, NULL, NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_bucket to fail with NULL name, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_bucket(&zeroapi, "aname", NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_bucket to fail with uninitialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_bucket(&api, "aname", NULL, 0) < 0) {
      fprintf(stderr, "Failed to create bucket: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    if (deltacloud_get_bucket_by_id(&api, "aname", &bucket) < 0) {
      fprintf(stderr, "Failed to get the bucket by name: %s\n",
	      deltacloud_get_last_error_string());
      /* FIXME: unfortunately, if we get here, then we can no longer destroy
       * the bucket that we created above.  What to do?
       */
      goto cleanup;
    }

    if (deltacloud_bucket_create_blob_from_file(NULL, &bucket, "ablob", "tmp",
						NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_create_blob_from_file to fail with NULL api, but succeeded\n");
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_create_blob_from_file(&api, NULL, "ablob", "tmp",
						NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_create_blob_from_file to fail with NULL bucket, but succeeded\n");
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_create_blob_from_file(&api, &bucket, NULL, "tmp",
						NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_create_blob_from_file to fail with NULL name, but succeeded\n");
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_create_blob_from_file(&api, &bucket, "ablob", NULL,
						NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_create_blob_from_file to fail with NULL file, but succeeded\n");
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_create_blob_from_file(&zeroapi, &bucket, "ablob",
						"tmp", NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_create_blob_from_file to fail with uninitialized API, but succeeded\n");
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    fp = fopen("tmp", "w");
    if (fp == NULL) {
      fprintf(stderr, "Failed to open file tmp: %s\n", strerror(errno));
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }
    fprintf(fp, "hello there, this is a blob for a bucket\n");
    fclose(fp);

    if (deltacloud_bucket_create_blob_from_file(&api, &bucket, "ablob", "tmp",
						NULL, 0) < 0) {
      fprintf(stderr, "Failed to create a blob from file: %s\n",
	      deltacloud_get_last_error_string());
      unlink("tmp");
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    unlink("tmp");

    if (deltacloud_bucket_get_blob_by_id(NULL, &bucket, "ablob", &blob) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_get_blob_by_id to fail with NULL api, but succeeded\n");
      /* FIXME: should we try to delete the blob? */
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_get_blob_by_id(&api, NULL, "ablob", &blob) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_get_blob_by_id to fail with NULL bucket, but succeeded\n");
      /* FIXME: should we try to delete the blob? */
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_get_blob_by_id(&api, &bucket, NULL, &blob) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_get_blob_by_id to fail with NULL name, but succeeded\n");
      /* FIXME: should we try to delete the blob? */
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_get_blob_by_id(&api, &bucket, "ablob", NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_get_blob_by_id to fail with NULL blob, but succeeded\n");
      /* FIXME: should we try to delete the blob? */
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_get_blob_by_id(&zeroapi, &bucket, "ablob",
					 &blob) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_get_blob_by_id to fail with uninitialized api, but succeeded\n");
      /* FIXME: should we try to delete the blob? */
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_get_blob_by_id(&api, &bucket, "ablob", &blob) < 0) {
      fprintf(stderr, "Failed to get blob ablob by id: %s\n",
	      deltacloud_get_last_error_string());
      /* FIXME: if we couldn't get a handle to the blob, then we can't delete
       * it.  What to do here?
       */
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_blob_update_metadata(NULL, &blob,
					       stackparams, 2) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_blob_update_metadata to fail with NULL api, but succeeded\n");
      deltacloud_bucket_delete_blob(&api, &blob);
      deltacloud_free_bucket_blob(&blob);
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_blob_update_metadata(&api, NULL,
					       stackparams, 2) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_blob_update_metadata to fail with NULL blob, but succeeded\n");
      deltacloud_bucket_delete_blob(&api, &blob);
      deltacloud_free_bucket_blob(&blob);
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_blob_update_metadata(&zeroapi, &blob,
					       stackparams, 2) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_blob_update_metadata to fail with uninitialized api, but succeeded\n");
      deltacloud_bucket_delete_blob(&api, &blob);
      deltacloud_free_bucket_blob(&blob);
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_prepare_parameter(&stackparams[0],
				     "X-Deltacloud-Blobmeta-name",
				     "foo") < 0) {
      fprintf(stderr, "Failed deltacloud_prepare_parameter: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_bucket_delete_blob(&api, &blob);
      deltacloud_free_bucket_blob(&blob);
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }
    if (deltacloud_prepare_parameter(&stackparams[1],
				     "X-Deltacloud-Blobmeta-bob",
				     "george") < 0) {
      fprintf(stderr, "Failed deltacloud_prepare_parameter: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_parameter_value(&stackparams[0]);
      deltacloud_bucket_delete_blob(&api, &blob);
      deltacloud_free_bucket_blob(&blob);
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_blob_update_metadata(&api, &blob,
					       stackparams, 2) < 0) {
      fprintf(stderr, "Failed deltacloud_bucket_blob_update_metadata: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_parameter_value(&stackparams[0]);
      deltacloud_free_parameter_value(&stackparams[1]);
      deltacloud_bucket_delete_blob(&api, &blob);
      deltacloud_free_bucket_blob(&blob);
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }
    deltacloud_free_parameter_value(&stackparams[0]);
    deltacloud_free_parameter_value(&stackparams[1]);

    if (deltacloud_bucket_delete_blob(NULL, &blob) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_delete_blob to fail with NULL api, but succeeded\n");
      /* FIXME: should we try to delete the blob? */
      deltacloud_free_bucket_blob(&blob);
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_delete_blob(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_delete_blob to fail with NULL blob, but succeeded\n");
      /* FIXME: should we try to delete the blob? */
      deltacloud_free_bucket_blob(&blob);
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_delete_blob(&zeroapi, &blob) >= 0) {
      fprintf(stderr, "Expected deltacloud_bucket_delete_blob to fail with uninitialized api, but succeeded\n");
      /* FIXME: should we try to delete the blob? */
      deltacloud_free_bucket_blob(&blob);
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
    }

    if (deltacloud_bucket_delete_blob(&api, &blob) < 0) {
      fprintf(stderr, "Failed to delete blob: %s\n",
	      deltacloud_get_last_error_string());
      /* FIXME: if the delete failed, then destroying the bucket is going to
       * fail since it still has stuff in it.  What to do?
       */
      deltacloud_free_bucket_blob(&blob);
      deltacloud_bucket_destroy(&api, &bucket);
      deltacloud_free_bucket(&bucket);
      goto cleanup;
   }

    deltacloud_bucket_destroy(&api, &bucket);
    deltacloud_free_bucket(&bucket);
  }
  else
    fprintf(stderr, "Buckets are not supported\n");

 cleanup:
  deltacloud_free_bucket_list(&buckets);

  deltacloud_free(&api);

  return ret;
}
