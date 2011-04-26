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
#include "instance_state.h"

static void free_transition(struct transition *transition)
{
  SAFE_FREE(transition->action);
  SAFE_FREE(transition->to);
  SAFE_FREE(transition->auto_bool);
}

int add_to_transition_list(struct transition **transitions, const char *action,
			   const char *to, const char *auto_bool)
{
  struct transition *onetransition;

  onetransition = malloc(sizeof(struct transition));
  if (onetransition == NULL)
    return -1;

  memset(onetransition, 0, sizeof(struct transition));

  if (strdup_or_null(&onetransition->action, action) < 0)
    goto error;
  if (strdup_or_null(&onetransition->to, to) < 0)
    goto error;
  if (strdup_or_null(&onetransition->auto_bool, auto_bool) < 0)
    goto error;
  onetransition->next = NULL;

  add_to_list(transitions, struct transition, onetransition);

  return 0;

 error:
  free_transition(onetransition);
  SAFE_FREE(onetransition);
  return -1;
}

static int copy_transition(struct transition **dst, struct transition *curr)
{
  return add_to_transition_list(dst, curr->action, curr->to, curr->auto_bool);
}

static int copy_transition_list(struct transition **dst,
				struct transition **src)
{
  copy_list(dst, src, struct transition, copy_transition, free_transition_list);
}

void free_transition_list(struct transition **transitions)
{
  free_list(transitions, struct transition, free_transition);
}

int add_to_instance_state_list(struct deltacloud_instance_state **instance_states,
			       const char *name,
			       struct transition *transitions)
{
  struct deltacloud_instance_state *oneinstance_state;

  oneinstance_state = malloc(sizeof(struct deltacloud_instance_state));
  if (oneinstance_state == NULL)
    return -1;

  memset(oneinstance_state, 0, sizeof(struct deltacloud_instance_state));

  if (strdup_or_null(&oneinstance_state->name, name) < 0)
    goto error;
  if (copy_transition_list(&oneinstance_state->transitions, &transitions) < 0)
    goto error;
  oneinstance_state->next = NULL;

  add_to_list(instance_states, struct deltacloud_instance_state,
	      oneinstance_state);

  return 0;

 error:
  deltacloud_free_instance_state(oneinstance_state);
  SAFE_FREE(oneinstance_state);
  return -1;
}

struct deltacloud_instance_state *find_by_name_in_instance_state_list(struct deltacloud_instance_state **instance_states,
							   const char *name)
{
  struct deltacloud_instance_state *curr;

  curr = *instance_states;
  while (curr != NULL) {
    if (STREQ(curr->name, name))
      return curr;
    curr = curr->next;
  }

  return NULL;
}

int copy_instance_state(struct deltacloud_instance_state *dst,
			struct deltacloud_instance_state *src)
{
  /* with a NULL src, we just return success.  A NULL dst is an error */
  if (src == NULL)
    return 0;
  if (dst == NULL)
    return -1;

  memset(dst, 0, sizeof(struct deltacloud_instance_state));

  if (strdup_or_null(&dst->name, src->name) < 0)
    goto error;
  if (copy_transition_list(&dst->transitions, &src->transitions) < 0)
    goto error;
  dst->next = NULL;

  return 0;

 error:
  deltacloud_free_instance_state(dst);
  return -1;
}

void deltacloud_free_instance_state(struct deltacloud_instance_state *instance_state)
{
  if (instance_state == NULL)
    return;

  SAFE_FREE(instance_state->name);
  free_transition_list(&instance_state->transitions);
}

void deltacloud_free_instance_state_list(struct deltacloud_instance_state **instance_states)
{
  free_list(instance_states, struct deltacloud_instance_state,
	    deltacloud_free_instance_state);
}
