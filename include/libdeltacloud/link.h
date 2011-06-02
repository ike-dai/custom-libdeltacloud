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

#ifndef LIBDELTACLOUD_LINK_H
#define LIBDELTACLOUD_LINK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing a constraint on a feature.
 */
struct deltacloud_feature_constraint {
  char *name; /**< The name of the constraint */
  char *value; /**< The value of the constraint */

  struct deltacloud_feature_constraint *next;
};

/**
 * A structure representing a single feature.
 */
struct deltacloud_feature {
  char *name; /**< The name of the feature */
  struct deltacloud_feature_constraint *constraints; /**< A list of constraints associated with this feature */

  struct deltacloud_feature *next;
};

/**
 * A structure representing a single link.
 */
struct deltacloud_link {
  char *href; /**< The full URL to the resource */
  char *rel; /**< The relative name of the resource */
  struct deltacloud_feature *features; /**< A list of features associated with the link */

  struct deltacloud_link *next;
};

void free_link_list(struct deltacloud_link **links);

#ifdef __cplusplus
}
#endif

#endif
