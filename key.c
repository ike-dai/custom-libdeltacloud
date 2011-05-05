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

int add_to_key_list(struct deltacloud_key **keys, const char *href,
		    const char *id, const char *type, const char *state,
		    const char *fingerprint)
{
  struct deltacloud_key *onekey;

  onekey = malloc(sizeof(struct deltacloud_key));
  if (onekey == NULL)
    return -1;

  memset(onekey, 0, sizeof(struct deltacloud_key));

  if (strdup_or_null(&onekey->href, href) < 0)
    goto error;
  if (strdup_or_null(&onekey->id, id) < 0)
    goto error;
  if (strdup_or_null(&onekey->type, type) < 0)
    goto error;
  if (strdup_or_null(&onekey->state, state) < 0)
    goto error;
  if (strdup_or_null(&onekey->fingerprint, fingerprint) < 0)
    goto error;

  onekey->next = NULL;

  add_to_list(keys, struct deltacloud_key, onekey);

  return 0;

 error:
  deltacloud_free_key(onekey);
  SAFE_FREE(onekey);
  return -1;
}

int copy_key(struct deltacloud_key *dst, struct deltacloud_key *src)
{
  /* with a NULL src, we just return success.  A NULL dst is an error */
  if (src == NULL)
    return 0;
  if (dst == NULL)
    return -1;

  memset(dst, 0, sizeof(struct deltacloud_key));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->type, src->type) < 0)
    goto error;
  if (strdup_or_null(&dst->state, src->state) < 0)
    goto error;
  if (strdup_or_null(&dst->fingerprint, src->fingerprint) < 0)
    goto error;

  return 0;

 error:
  deltacloud_free_key(dst);
  return -1;
}

void deltacloud_free_key_list(struct deltacloud_key **keys)
{
  free_list(keys, struct deltacloud_key, deltacloud_free_key);
}

