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
#include "libdeltacloud.h"
#include "test_common.h"

static void print_key(struct deltacloud_key *key)
{
  fprintf(stderr, "Key %s\n", key->id);
  fprintf(stderr, "\tHref: %s\n", key->href);
  fprintf(stderr, "\tType: %s, State: %s\n", key->type, key->state);
  fprintf(stderr, "\tFingerprint: %s\n", key->fingerprint);
}

static void print_key_list(struct deltacloud_key *keys)
{
  struct deltacloud_key *key;

  deltacloud_for_each(key, keys)
    print_key(key);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_api zeroapi;
  struct deltacloud_key *keys = NULL;
  struct deltacloud_key key;
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

  /* test out deltacloud_supports_keys */
  if (deltacloud_supports_keys(NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_keys to fail with NULL api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_keys(&zeroapi) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_keys to fail with uninitialized api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_keys(&api)) {

    /* test out deltacloud_get_keys */
    if (deltacloud_get_keys(NULL, &keys) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_keys to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_keys(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_keys to fail with NULL keys, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_keys(&zeroapi, &keys) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_keys to fail with unintialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_keys(&api, &keys) < 0) {
      fprintf(stderr, "Failed to get keys: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_key_list(keys);

    if (keys != NULL) {

      /* test out deltacloud_get_key_by_id */
      if (deltacloud_get_key_by_id(NULL, keys->id, &key) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_key_by_id to fail with NULL api, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_key_by_id(&api, NULL, &key) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_key_by_id to fail with NULL id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_key_by_id(&api, keys->id, NULL) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_key_by_id to fail with NULL key, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_key_by_id(&api, "bogus_id", &key) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_key_by_id to fail with bogus id, but succeeded\n");
	goto cleanup;
      }

      if (deltacloud_get_key_by_id(&zeroapi, keys->id, &key) >= 0) {
	fprintf(stderr, "Expected deltacloud_get_key_by_id to fail with unintialized api, but succeeded\n");
	goto cleanup;
      }

      /* here we use the first key from the list above */
      if (deltacloud_get_key_by_id(&api, keys->id, &key) < 0) {
	fprintf(stderr, "Failed to get key by ID: %s\n",
		deltacloud_get_last_error_string());
	goto cleanup;
      }
      print_key(&key);
      deltacloud_free_key(&key);
    }

    /* test out deltacloud_create_key */
    if (deltacloud_create_key(NULL, "testkey", NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_key to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_key(&api, NULL, NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_key to fail with NULL key name, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_key(&zeroapi, "testkey", NULL, 0) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_key to fail with uninitialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_key(&api, "testkey", NULL, -1) >= 0) {
      fprintf(stderr, "Expected deltacloud_create_key to fail with negative params_length, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_create_key(&api, "testkey", NULL, 0) < 0) {
      fprintf(stderr, "Failed to create key: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    if (deltacloud_get_key_by_id(&api, "testkey", &key) < 0) {
      fprintf(stderr, "Failed to retrieve just created key: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_key(&key);

    /* test out deltacloud_key_destroy */
    if (deltacloud_key_destroy(NULL, &key) >= 0) {
      deltacloud_free_key(&key);
      fprintf(stderr, "Expected deltacloud_key_destroy to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_key_destroy(&api, NULL) >= 0) {
      deltacloud_free_key(&key);
      fprintf(stderr, "Expected deltacloud_key_destroy to fail with NULL key, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_key_destroy(&zeroapi, &key) >= 0) {
      deltacloud_free_key(&key);
      fprintf(stderr, "Expected deltacloud_key_destroy to fail with uninitialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_key_destroy(&api, &key) < 0) {
      deltacloud_free_key(&key);
      fprintf(stderr, "Failed to destroy key: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    deltacloud_free_key(&key);
  }
  else
    fprintf(stderr, "Keys are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free_key_list(&keys);

  deltacloud_free(&api);

  return ret;
}
