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
#include "action.h"

void free_action(struct deltacloud_action *action)
{
  SAFE_FREE(action->rel);
  SAFE_FREE(action->href);
  SAFE_FREE(action->method);
}

int add_to_action_list(struct deltacloud_action **actions,
		       struct deltacloud_action *action)
{
  struct deltacloud_action *oneaction;

  oneaction = calloc(1, sizeof(struct deltacloud_action));
  if (oneaction == NULL)
    return -1;

  if (strdup_or_null(&oneaction->rel, action->rel) < 0)
    goto error;
  if (strdup_or_null(&oneaction->href, action->href) < 0)
    goto error;
  if (strdup_or_null(&oneaction->method, action->method) < 0)
    goto error;

  add_to_list(actions, struct deltacloud_action, oneaction);

  return 0;

 error:
  free_action(oneaction);
  SAFE_FREE(oneaction);
  return -1;
}

int copy_action_list(struct deltacloud_action **dst,
		     struct deltacloud_action **src)
{
  copy_list(dst, src, struct deltacloud_action, add_to_action_list,
	    free_action_list);
}

void free_action_list(struct deltacloud_action **actions)
{
  free_list(actions, struct deltacloud_action, free_action);
}
