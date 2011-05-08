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
#include "libdeltacloud.h"

int parse_actions_xml(xmlNodePtr root, struct deltacloud_action **actions)
{
  struct deltacloud_action *thisaction;
  xmlNodePtr cur;
  int ret = -1;

  cur = root->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "link")) {

      thisaction = calloc(1, sizeof(struct deltacloud_action));
      if (thisaction == NULL) {
	oom_error();
	goto cleanup;
      }

      thisaction->href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thisaction->rel = (char *)xmlGetProp(cur, BAD_CAST "rel");
      thisaction->method = (char *)xmlGetProp(cur, BAD_CAST "method");

      /* add_to_list can't fail */
      add_to_list(actions, struct deltacloud_action, thisaction);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_action_list(actions);

  return ret;
}

static void free_action(struct deltacloud_action *action)
{
  SAFE_FREE(action->rel);
  SAFE_FREE(action->href);
  SAFE_FREE(action->method);
}

void free_action_list(struct deltacloud_action **actions)
{
  free_list(actions, struct deltacloud_action, free_action);
}
