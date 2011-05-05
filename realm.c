/*
 * Copyright (C) 2010,2011 Red Hat, Inc.
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
#include "realm.h"

int copy_realm(struct deltacloud_realm *dst, struct deltacloud_realm *src)
{
  /* with a NULL src, we just return success.  A NULL dst is an error */
  if (src == NULL)
    return 0;
  if (dst == NULL)
    return -1;

  memset(dst, 0, sizeof(struct deltacloud_realm));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->name, src->name) < 0)
    goto error;
  if (strdup_or_null(&dst->state, src->state) < 0)
    goto error;
  if (strdup_or_null(&dst->limit, src->limit) < 0)
    goto error;

  return 0;

 error:
  deltacloud_free_realm(dst);
  return -1;
}

int add_to_realm_list(struct deltacloud_realm **realms,
		      struct deltacloud_realm *realm)
{
  struct deltacloud_realm *onerealm;

  onerealm = calloc(1, sizeof(struct deltacloud_realm));
  if (onerealm == NULL)
    return -1;

  if (copy_realm(onerealm, realm) < 0)
    goto error;

  add_to_list(realms, struct deltacloud_realm, onerealm);

  return 0;

 error:
  deltacloud_free_realm(onerealm);
  SAFE_FREE(onerealm);
  return -1;
}

void deltacloud_free_realm(struct deltacloud_realm *realm)
{
  if (realm == NULL)
    return;

  SAFE_FREE(realm->href);
  SAFE_FREE(realm->id);
  SAFE_FREE(realm->name);
  SAFE_FREE(realm->limit);
  SAFE_FREE(realm->state);
}

void deltacloud_free_realm_list(struct deltacloud_realm **realms)
{
  free_list(realms, struct deltacloud_realm, deltacloud_free_realm);
}
