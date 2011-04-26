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

static void free_feature(struct deltacloud_feature *feature)
{
  SAFE_FREE(feature->name);
}

int add_to_feature_list(struct deltacloud_feature **features,
			char *name)
{
  struct deltacloud_feature *onefeature;

  onefeature = calloc(1, sizeof(struct deltacloud_feature));
  if (onefeature == NULL)
    return -1;

  if (strdup_or_null(&onefeature->name, name) < 0)
    goto error;

  add_to_list(features, struct deltacloud_feature, onefeature);

  return 0;

 error:
  free_feature(onefeature);
  SAFE_FREE(onefeature);
  return -1;
}

static int copy_feature(struct deltacloud_feature **dst,
			struct deltacloud_feature *curr)
{
  return add_to_feature_list(dst, curr->name);
}

static int copy_feature_list(struct deltacloud_feature **dst,
                     struct deltacloud_feature **src)
{
  copy_list(dst, src, struct deltacloud_feature, copy_feature,
           free_feature_list);
}

void free_feature_list(struct deltacloud_feature **features)
{
  free_list(features, struct deltacloud_feature, free_feature);
}

static void free_link(struct deltacloud_link *link)
{
  SAFE_FREE(link->href);
  SAFE_FREE(link->rel);
  free_feature_list(&link->features);
}

int add_to_link_list(struct deltacloud_link **links, char *href, char *rel,
		     struct deltacloud_feature *features)
{
  struct deltacloud_link *onelink;

  onelink = malloc(sizeof(struct deltacloud_link));
  if (onelink == NULL)
    return -1;

  memset(onelink, 0, sizeof(struct deltacloud_link));

  if (strdup_or_null(&onelink->href, href) < 0)
    goto error;
  if (strdup_or_null(&onelink->rel, rel) < 0)
    goto error;
  if (copy_feature_list(&onelink->features, &features) < 0)
    goto error;
  onelink->next = NULL;

  add_to_list(links, struct deltacloud_link, onelink);

  return 0;

 error:
  free_link(onelink);
  SAFE_FREE(onelink);
  return -1;
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
  free_list(links, struct deltacloud_link, free_link);
}
