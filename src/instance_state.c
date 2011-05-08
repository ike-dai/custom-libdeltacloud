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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "libdeltacloud.h"

static void free_transition(struct deltacloud_instance_state_transition *transition)
{
  SAFE_FREE(transition->action);
  SAFE_FREE(transition->to);
  SAFE_FREE(transition->auto_bool);
}

static void free_transition_list(struct deltacloud_instance_state_transition **transitions)
{
  free_list(transitions, struct deltacloud_instance_state_transition,
	    free_transition);
}

static void free_instance_state(struct deltacloud_instance_state *instance_state)
{
  if (instance_state == NULL)
    return;

  SAFE_FREE(instance_state->name);
  free_transition_list(&instance_state->transitions);
}

static int parse_one_instance_state(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void *output)
{
  struct deltacloud_instance_state *thisstate = (struct deltacloud_instance_state *)output;
  struct deltacloud_instance_state_transition *thistrans;
  xmlNodePtr state_cur;

  thisstate->name = (char *)xmlGetProp(cur, BAD_CAST "name");

  state_cur = cur->children;
  while (state_cur != NULL) {
    if (state_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)state_cur->name, "transition")) {

      thistrans = calloc(1,
			 sizeof(struct deltacloud_instance_state_transition));
      if (thistrans == NULL) {
	oom_error();
	free_instance_state(thisstate);
	return -1;
      }

      thistrans->action = (char *)xmlGetProp(state_cur, BAD_CAST "action");
      thistrans->to = (char *)xmlGetProp(state_cur, BAD_CAST "to");
      thistrans->auto_bool = (char *)xmlGetProp(state_cur, BAD_CAST "auto");

      /* add_to_list can't fail */
      add_to_list(&thisstate->transitions,
		  struct deltacloud_instance_state_transition, thistrans);
    }
    state_cur = state_cur->next;
  }

  return 0;
}

static int parse_instance_state_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void **data)
{
  struct deltacloud_instance_state **instance_states = (struct deltacloud_instance_state **)data;
  struct deltacloud_instance_state *thisstate;
  int ret = -1;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "state")) {

      thisstate = calloc(1, sizeof(struct deltacloud_instance_state));
      if (thisstate == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_instance_state(cur, ctxt, thisstate) < 0) {
	/* parse_one_instance_state is expected to have set its own error */
	SAFE_FREE(thisstate);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(instance_states, struct deltacloud_instance_state, thisstate);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_instance_state_list(instance_states);

  return ret;
}

int deltacloud_get_instance_states(struct deltacloud_api *api,
				   struct deltacloud_instance_state **instance_states)
{
  return internal_get(api, "instance_states", "states",
		      parse_instance_state_xml, (void **)instance_states);
}

void deltacloud_free_instance_state_list(struct deltacloud_instance_state **instance_states)
{
  free_list(instance_states, struct deltacloud_instance_state,
	    free_instance_state);
}
