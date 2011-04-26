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

#ifndef INSTANCE_STATE_H
#define INSTANCE_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

struct transition {
  char *action;
  char *to;
  char *auto_bool;

  struct transition *next;
};

struct deltacloud_instance_state {
  char *name;
  struct transition *transitions;

  struct deltacloud_instance_state *next;
};

int add_to_transition_list(struct transition **transitions, const char *action,
			   const char *to, const char *auto_bool);
void free_transition_list(struct transition **transitions);

int add_to_instance_state_list(struct deltacloud_instance_state **instance_states,
			       const char *name,
			       struct transition *transitions);
struct deltacloud_instance_state *find_by_name_in_instance_state_list(struct deltacloud_instance_state **instance_states,
							   const char *name);
int copy_instance_state(struct deltacloud_instance_state *dst,
			struct deltacloud_instance_state *src);
void deltacloud_free_instance_state(struct deltacloud_instance_state *instance_state);
void deltacloud_free_instance_state_list(struct deltacloud_instance_state **instance_states);

#ifdef __cplusplus
}
#endif

#endif
