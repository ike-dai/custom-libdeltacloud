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

void free_transition(struct deltacloud_instance_state_transition *transition)
{
  SAFE_FREE(transition->action);
  SAFE_FREE(transition->to);
  SAFE_FREE(transition->auto_bool);
}

int add_to_transition_list(struct deltacloud_instance_state_transition **transitions,
			   struct deltacloud_instance_state_transition *transition)
{
  struct deltacloud_instance_state_transition *onetransition;

  onetransition = calloc(1,
			 sizeof(struct deltacloud_instance_state_transition));
  if (onetransition == NULL)
    return -1;

  if (strdup_or_null(&onetransition->action, transition->action) < 0)
    goto error;
  if (strdup_or_null(&onetransition->to, transition->to) < 0)
    goto error;
  if (strdup_or_null(&onetransition->auto_bool, transition->auto_bool) < 0)
    goto error;

  add_to_list(transitions, struct deltacloud_instance_state_transition,
	      onetransition);

  return 0;

 error:
  free_transition(onetransition);
  SAFE_FREE(onetransition);
  return -1;
}

static int copy_transition_list(struct deltacloud_instance_state_transition **dst,
				struct deltacloud_instance_state_transition **src)
{
  copy_list(dst, src, struct deltacloud_instance_state_transition,
	    add_to_transition_list, free_transition_list);
}

void free_transition_list(struct deltacloud_instance_state_transition **transitions)
{
  free_list(transitions, struct deltacloud_instance_state_transition,
	    free_transition);
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

int add_to_instance_state_list(struct deltacloud_instance_state **instance_states,
			       struct deltacloud_instance_state *state)
{
  struct deltacloud_instance_state *oneinstance_state;

  oneinstance_state = calloc(1, sizeof(struct deltacloud_instance_state));
  if (oneinstance_state == NULL)
    return -1;

  if (copy_instance_state(oneinstance_state, state) < 0)
     goto error;

  add_to_list(instance_states, struct deltacloud_instance_state,
	      oneinstance_state);

  return 0;

 error:
  deltacloud_free_instance_state(oneinstance_state);
  SAFE_FREE(oneinstance_state);
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
