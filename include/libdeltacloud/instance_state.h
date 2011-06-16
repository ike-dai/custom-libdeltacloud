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

#ifndef LIBDELTACLOUD_INSTANCE_STATE_H
#define LIBDELTACLOUD_INSTANCE_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing a single instance state transition.
 */
struct deltacloud_instance_state_transition {
  char *action; /**< The name of this action */
  char *to; /**< The state that can be transitioned to */
  char *automatically; /**< Whether the state transition happens automatically */

  struct deltacloud_instance_state_transition *next;
};

/**
 * A structure representing a single instance state (RUNNING, STOPPED, etc).
 */
struct deltacloud_instance_state {
  char *name; /**< The name of the instance state */
  struct deltacloud_instance_state_transition *transitions; /**< A list of the valid transitions from this state */

  struct deltacloud_instance_state *next;
};

#define deltacloud_supports_instance_states(api) deltacloud_has_link(api, "instance_states")
int deltacloud_get_instance_states(struct deltacloud_api *api,
				   struct deltacloud_instance_state **instance_states);
void deltacloud_free_instance_state_list(struct deltacloud_instance_state **instance_states);

#ifdef __cplusplus
}
#endif

#endif
