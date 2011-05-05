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

#ifndef LIBDELTACLOUD_ACTION_H
#define LIBDELTACLOUD_ACTION_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_action {
  char *rel;
  char *href;
  char *method;

  struct deltacloud_action *next;
};

void free_action(struct deltacloud_action *action);
int add_to_action_list(struct deltacloud_action **actions,
		       struct deltacloud_action *action);
int copy_action_list(struct deltacloud_action **dst,
		     struct deltacloud_action **src);
void free_action_list(struct deltacloud_action **actions);

#ifdef __cplusplus
}
#endif

#endif
