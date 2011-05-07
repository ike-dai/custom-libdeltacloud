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

static void find_by_name_error(const char *name, const char *type)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Failed to find '%s' in '%s' list", name, type) < 0) {
    tmp = "Failed to find the link";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_NAME_NOT_FOUND_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void free_transition(struct deltacloud_instance_state_transition *transition)
{
  SAFE_FREE(transition->action);
  SAFE_FREE(transition->to);
  SAFE_FREE(transition->auto_bool);
}

static int copy_transition(struct deltacloud_instance_state_transition **dst,
			   struct deltacloud_instance_state_transition *src)
{
  memset(dst, 0, sizeof(struct deltacloud_instance_state_transition));

  if (strdup_or_null(&(*dst)->action, src->action) < 0)
    goto error;
  if (strdup_or_null(&(*dst)->to, src->to) < 0)
    goto error;
  if (strdup_or_null(&(*dst)->auto_bool, src->auto_bool) < 0)
    goto error;

  return 0;

 error:
  free_transition(*dst);
  return -1;
}

static void free_transition_list(struct deltacloud_instance_state_transition **transitions)
{
  free_list(transitions, struct deltacloud_instance_state_transition,
	    free_transition);
}

static int copy_transition_list(struct deltacloud_instance_state_transition **dst,
				struct deltacloud_instance_state_transition **src)
{
  copy_list(dst, src, struct deltacloud_instance_state_transition,
	    copy_transition, free_transition_list);
}

static int copy_instance_state(struct deltacloud_instance_state *dst,
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

  return 0;

 error:
  deltacloud_free_instance_state(dst);
  return -1;
}

static int parse_instance_state_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void **data)
{
  struct deltacloud_instance_state **instance_states = (struct deltacloud_instance_state **)data;
  struct deltacloud_instance_state *thisstate;
  struct deltacloud_instance_state_transition *thistrans;
  xmlNodePtr state_cur;
  int ret = -1;

  memset(&thistrans, 0, sizeof(struct deltacloud_instance_state_transition));

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "state")) {

      thisstate = calloc(1, sizeof(struct deltacloud_instance_state));
      if (thisstate == NULL) {
	oom_error();
	goto cleanup;
      }

      thisstate->name = (char *)xmlGetProp(cur, BAD_CAST "name");

      state_cur = cur->children;
      while (state_cur != NULL) {
	if (state_cur->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)state_cur->name, "transition")) {

	  thistrans = calloc(1,
			     sizeof(struct deltacloud_instance_state_transition));
	  if (thistrans == NULL) {
	    oom_error();
	    deltacloud_free_instance_state(thisstate);
	    SAFE_FREE(thisstate);
	    goto cleanup;
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

int deltacloud_get_instance_state_by_name(struct deltacloud_api *api,
					  const char *name,
					  struct deltacloud_instance_state *instance_state)
{
  struct deltacloud_instance_state *statelist = NULL;
  struct deltacloud_instance_state *state;
  int instance_ret;
  int ret = -1;

  /* despite the fact that 'name' is an input from the user, we don't
   * need to escape it since we are never using it as a URL
   */

  if (!valid_arg(api) || !valid_arg(name) || !valid_arg(instance_state))
    return -1;

  instance_ret = deltacloud_get_instance_states(api, &statelist);
  if (instance_ret < 0)
    return instance_ret;

  deltacloud_for_each(state, statelist) {
    if (STREQ(state->name, name))
      break;
  }
  if (state == NULL) {
    find_by_name_error(name, "instance_state");
    goto cleanup;
  }

  if (copy_instance_state(instance_state, state) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_instance_state_list(&statelist);
  return ret;
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
