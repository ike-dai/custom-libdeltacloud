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
#include "link.h"

static void free_link(struct deltacloud_link *link)
{
  SAFE_FREE(link->href);
  SAFE_FREE(link->rel);
}

int add_to_link_list(struct deltacloud_link **links, char *href, char *rel)
{
  struct deltacloud_link *onelink, *curr, *last;

  onelink = malloc(sizeof(struct deltacloud_link));
  if (onelink == NULL)
    return -1;

  memset(onelink, 0, sizeof(struct deltacloud_link));

  if (strdup_or_null(&onelink->href, href) < 0)
    goto error;
  if (strdup_or_null(&onelink->rel, rel) < 0)
    goto error;
  onelink->next = NULL;

  if (*links == NULL)
    /* First element in the list */
    *links = onelink;
  else {
    curr = *links;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = onelink;
  }

  return 0;

 error:
  free_link(onelink);
  SAFE_FREE(onelink);
  return -1;
}

void deltacloud_print_link(struct deltacloud_link *link, FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  if (link == NULL)
    return;

  fprintf(stream, "Href: %s\n", link->href);
  fprintf(stream, "ID: %s\n", link->rel);
}

void deltacloud_print_link_list(struct deltacloud_link **links, FILE *stream)
{
  print_list(links, struct deltacloud_link, deltacloud_print_link, stream);
}

struct deltacloud_link *find_by_rel_in_link_list(struct deltacloud_link **links,
						 char *rel)
{
  struct deltacloud_link *curr;

  curr = *links;
  while (curr != NULL) {
    if (STREQ(curr->rel, rel))
      return curr;
    curr = curr->next;
  }

  return NULL;
}

void free_link_list(struct deltacloud_link **links)
{
  struct deltacloud_link *curr, *next;

  curr = *links;
  while (curr != NULL) {
    next = curr->next;
    free_link(curr);
    SAFE_FREE(curr);
    curr = next;
  }

  *links = NULL;
}
