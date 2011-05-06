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
  struct deltacloud_key *keys;
  struct deltacloud_key key;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }

  if (deltacloud_supports_keys(&api)) {
    if (deltacloud_get_keys(&api, &keys) < 0) {
      fprintf(stderr, "Failed to get keys: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_key_list(keys);

    if (keys != NULL) {
      /* here we use the first key from the list above */
      if (deltacloud_get_key_by_id(&api, keys->id, &key) < 0) {
	fprintf(stderr, "Failed to get key by ID: %s\n",
		deltacloud_get_last_error_string());
	deltacloud_free_key_list(&keys);
	goto cleanup;
      }
      print_key(&key);
      deltacloud_free_key(&key);
    }

    deltacloud_free_key_list(&keys);

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
    deltacloud_key_destroy(&api, &key);
    deltacloud_free_key(&key);
  }
  else
    fprintf(stderr, "Keys are not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
