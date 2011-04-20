/*
 * Copyright (C) 2010 Red Hat, Inc.
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

int add_to_realm_list(struct deltacloud_realm **realms, char *href, char *id,
		      char *name, char *state, char *limit)
{
  struct deltacloud_realm *onerealm, *now, *last;

  onerealm = malloc(sizeof(struct deltacloud_realm));
  if (onerealm == NULL)
    return -1;

  memset(onerealm, 0, sizeof(struct deltacloud_realm));

  if (strdup_or_null(&onerealm->href, href) < 0)
    goto error;
  if (strdup_or_null(&onerealm->id, id) < 0)
    goto error;
  if (strdup_or_null(&onerealm->name, name) < 0)
    goto error;
  if (strdup_or_null(&onerealm->state, state) < 0)
    goto error;
  if (strdup_or_null(&onerealm->limit, limit) < 0)
    goto error;
  onerealm->next = NULL;

  if (*realms == NULL)
    /* First element in the list */
    *realms = onerealm;
  else {
    now = *realms;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = onerealm;
  }

  return 0;

 error:
  deltacloud_free_realm(onerealm);
  SAFE_FREE(onerealm);
  return -1;
}

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
  dst->next = NULL;

  return 0;

 error:
  deltacloud_free_realm(dst);
  return -1;
}

void deltacloud_print_realm(struct deltacloud_realm *realm, FILE *stream)
{
  if (stream == NULL)
    stream = stderr;
  if (realm == NULL)
    return;

  fprintf(stream, "Href: %s\n", realm->href);
  fprintf(stream, "ID: %s\n", realm->id);
  fprintf(stream, "Name: %s\n", realm->name);
  fprintf(stream, "Limit: %s\n", realm->limit);
  fprintf(stream, "State: %s\n", realm->state);
}

void deltacloud_print_realm_list(struct deltacloud_realm **realms, FILE *stream)
{
  print_list(realms, struct deltacloud_realm, deltacloud_print_realm, stream);
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
  struct deltacloud_realm *curr, *next;

  if (realms == NULL)
    return;

  curr = *realms;
  while (curr != NULL) {
    next = curr->next;
    deltacloud_free_realm(curr);
    SAFE_FREE(curr);
    curr = next;
  }

  *realms = NULL;
}
