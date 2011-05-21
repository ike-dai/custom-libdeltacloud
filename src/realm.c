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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "realm.h"

/** @file */

static int parse_one_realm(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			   void *output)
{
  struct deltacloud_realm *thisrealm = (struct deltacloud_realm *)output;

  memset(thisrealm, 0, sizeof(struct deltacloud_realm));

  thisrealm->href = (char *)xmlGetProp(cur, BAD_CAST "href");
  thisrealm->id = (char *)xmlGetProp(cur, BAD_CAST "id");
  thisrealm->name = getXPathString("string(./name)", ctxt);
  thisrealm->state = getXPathString("string(./state)", ctxt);
  thisrealm->limit = getXPathString("string(./limit)", ctxt);

  return 0;
}

static int parse_realm_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			   void **data)
{
  struct deltacloud_realm **realms = (struct deltacloud_realm **)data;
  struct deltacloud_realm *thisrealm;
  xmlNodePtr oldnode;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "realm")) {

      ctxt->node = cur;

      thisrealm = calloc(1, sizeof(struct deltacloud_realm));
      if (thisrealm == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_realm(cur, ctxt, thisrealm) < 0) {
	/* parse_one_realm is expected to have set its own error */
	SAFE_FREE(thisrealm);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(realms, struct deltacloud_realm, thisrealm);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_realm_list(realms);
  ctxt->node = oldnode;

  return ret;
}

/**
 * A function to get a linked list of all of the realms.  The caller
 * is expected to free the list using deltacloud_free_realm_list().
 * @param[in] api The deltacloud_api structure representing this connection
 * @param[out] realms A pointer to the deltacloud_realm structure to hold the
 *                    list of realms
 * @returns 0 on success, -1 on error
 */
int deltacloud_get_realms(struct deltacloud_api *api,
			  struct deltacloud_realm **realms)
{
  return internal_get(api, "realms", "realms", parse_realm_xml,
		      (void **)realms);
}

/**
 * A function to look up a particular realm by id.  The caller is expected
 * to free the deltacloud_realm structure using deltacloud_free_realm().
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] id The realm ID to look for
 * @param[out] realm The deltacloud_realm structure to fill in if the ID is
 *                   found
 * @returns 0 on success, -1 if the realm cannot be found or on error
 */
int deltacloud_get_realm_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_realm *realm)
{
  return internal_get_by_id(api, id, "realms", "realm", parse_one_realm, realm);
}

/**
 * A function to free a deltacloud_realm structure initially allocated
 * by deltacloud_get_realm_by_id().
 * @param[in] realm The deltacloud_realm structure representing the realm
 */
void deltacloud_free_realm(struct deltacloud_realm *realm)
{
  if (realm == NULL)
    return;

  SAFE_FREE(realm->href);
  SAFE_FREE(realm->id);
  SAFE_FREE(realm->name);
  SAFE_FREE(realm->limit);
  SAFE_FREE(realm->state);
}

/**
 * A function to free a list of deltacloud_realm structures initially
 * allocated by deltacloud_get_realms().
 * @param[in] realms The pointer to the head of the deltacloud_realm list
 */
void deltacloud_free_realm_list(struct deltacloud_realm **realms)
{
  free_list(realms, struct deltacloud_realm, deltacloud_free_realm);
}
