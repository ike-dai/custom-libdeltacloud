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
#include "link.h"

static void free_constraint(struct deltacloud_feature_constraint *constraint)
{
  SAFE_FREE(constraint->name);
  SAFE_FREE(constraint->value);
}

static int parse_constraint_xml(xmlNodePtr constraintnode,
				struct deltacloud_feature_constraint **constraints)
{
  struct deltacloud_feature_constraint *thisconstraint;

  while (constraintnode != NULL) {
    if (constraintnode->type == XML_ELEMENT_NODE &&
	STREQ((const char *)constraintnode->name, "constraint")) {

      thisconstraint = calloc(1, sizeof(struct deltacloud_feature_constraint));
      if (thisconstraint == NULL) {
	oom_error();
	return -1;
      }

      thisconstraint->name = (char *)xmlGetProp(constraintnode,
						BAD_CAST "name");
      thisconstraint->value = (char *)xmlGetProp(constraintnode,
						 BAD_CAST "value");

      /* add_to_list can't fail */
      add_to_list(constraints, struct deltacloud_feature_constraint,
		  thisconstraint);
    }
    constraintnode = constraintnode->next;
  }

  return 0;
}

static void free_feature(struct deltacloud_feature *feature)
{
  SAFE_FREE(feature->name);
  free_list(&feature->constraints, struct deltacloud_feature_constraint,
	    free_constraint);
}

static int parse_feature_xml(xmlNodePtr featurenode,
			     struct deltacloud_feature **features)
{
  struct deltacloud_feature *thisfeature;

  while (featurenode != NULL) {
    if (featurenode->type == XML_ELEMENT_NODE &&
	STREQ((const char *)featurenode->name, "feature")) {

      thisfeature = calloc(1, sizeof(struct deltacloud_feature));
      if (thisfeature == NULL) {
	oom_error();
	return -1;
      }

      thisfeature->name = (char *)xmlGetProp(featurenode, BAD_CAST "name");

      if (parse_constraint_xml(featurenode->children,
			       &(thisfeature->constraints)) < 0) {
	free_feature(thisfeature);
	SAFE_FREE(thisfeature);
	return -1;
      }

      /* add_to_list can't fail */
      add_to_list(features, struct deltacloud_feature, thisfeature);
    }
    featurenode = featurenode->next;
  }

  return 0;
}

static void free_link(struct deltacloud_link *link)
{
  SAFE_FREE(link->href);
  SAFE_FREE(link->rel);
  free_list(&link->features, struct deltacloud_feature, free_feature);
}

int parse_link_xml(xmlNodePtr linknode, struct deltacloud_link **links)
{
  struct deltacloud_link *thislink;
  int ret = -1;

  while (linknode != NULL) {
    if (linknode->type == XML_ELEMENT_NODE &&
	STREQ((const char *)linknode->name, "link")) {

      thislink = calloc(1, sizeof(struct deltacloud_link));
      if (thislink == NULL) {
	oom_error();
	goto cleanup;
      }

      thislink->href = (char *)xmlGetProp(linknode, BAD_CAST "href");
      thislink->rel = (char *)xmlGetProp(linknode, BAD_CAST "rel");
      if (parse_feature_xml(linknode->children, &(thislink->features)) < 0) {
	/* parse_feature_xml already set the error */
	free_link(thislink);
	SAFE_FREE(thislink);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(links, struct deltacloud_link, thislink);
    }
    linknode = linknode->next;
  }

  ret = 0;

 cleanup:
  return ret;
}

void free_link_list(struct deltacloud_link **links)
{
  free_list(links, struct deltacloud_link, free_link);
}
