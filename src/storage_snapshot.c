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
#include <memory.h>
#include "common.h"
#include "storage_snapshot.h"

/** @file */

static int parse_one_storage_snapshot(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void *output)
{
  struct deltacloud_storage_snapshot *thissnapshot = (struct deltacloud_storage_snapshot *)output;

  memset(thissnapshot, 0, sizeof(struct deltacloud_storage_snapshot));

  thissnapshot->href = (char *)xmlGetProp(cur, BAD_CAST "href");
  thissnapshot->id = (char *)xmlGetProp(cur, BAD_CAST "id");
  thissnapshot->created = getXPathString("string(./created)", ctxt);
  thissnapshot->state = getXPathString("string(./state)", ctxt);
  thissnapshot->storage_volume_href = getXPathString("string(./storage_volume/@href)",
						     ctxt);
  thissnapshot->storage_volume_id = getXPathString("string(./storage_volume/@id)",
						   ctxt);

  return 0;
}

static int parse_storage_snapshot_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data)
{
  struct deltacloud_storage_snapshot **storage_snapshots = (struct deltacloud_storage_snapshot **)data;
  struct deltacloud_storage_snapshot *thissnapshot;
  int ret = -1;
  xmlNodePtr oldnode;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "storage_snapshot")) {

      ctxt->node = cur;

      thissnapshot = calloc(1, sizeof(struct deltacloud_storage_snapshot));
      if (thissnapshot == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_storage_snapshot(cur, ctxt, thissnapshot) < 0) {
	/* parse_one_storage_snapshot is expected to have set its own error */
	SAFE_FREE(thissnapshot);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(storage_snapshots, struct deltacloud_storage_snapshot,
		  thissnapshot);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_storage_snapshot_list(storage_snapshots);

  return ret;
}

/**
 * A function to get a linked list of all of the storage snapshots.  The caller
 * is expected to free the list using deltacloud_free_storage_snapshot_list().
 * @param[in] api The deltacloud_api structure representing this connection
 * @param[out] storage_snapshots A pointer to the deltacloud_storage_snapshot
 *                               structure to hold the list of storage snapshots
 * @returns 0 on success, -1 on error
 */
int deltacloud_get_storage_snapshots(struct deltacloud_api *api,
				     struct deltacloud_storage_snapshot **storage_snapshots)
{
  return internal_get(api, "storage_snapshots", "storage_snapshots",
		      parse_storage_snapshot_xml, (void **)storage_snapshots);
}

/**
 * A function to look up a particular storage snapshot by id.  The caller is
 * expected to free the deltacloud_storage_snapshot structure using
 * deltacloud_free_storage_snapshot().
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] id The storage snapshot ID to look for
 * @param[out] storage_snapshot The deltacloud_storage_snapshot structure to
 *                              fill in if the ID is found
 * @returns 0 on success, -1 if the storage snapshot cannot be found or on error
 */
int deltacloud_get_storage_snapshot_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_storage_snapshot *storage_snapshot)
{
  return internal_get_by_id(api, id, "storage_snapshots", "storage_snapshot",
			    parse_one_storage_snapshot, storage_snapshot);
}

/**
 * A function to create a new storage snapshot.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] volume_id The volume ID to take the snapshot from
 * @param[in] params An array of deltacloud_create_parameter structures that
 *                   represent any optional parameters to pass into the
 *                   create call
 * @param[in] params_length An integer describing the length of the params
 *                          array
 * @returns 0 on success, -1 on error
 */
int deltacloud_create_storage_snapshot(struct deltacloud_api *api,
				       const char *volume_id,
				       struct deltacloud_create_parameter *params,
				       int params_length)
{
  struct deltacloud_create_parameter *internal_params;
  int ret = -1;
  int pos;

  if (!valid_api(api) || !valid_arg(volume_id))
    return -1;

  if (params_length < 0) {
    invalid_argument_error("params_length must be >= 0");
    return -1;
  }

  internal_params = calloc(params_length + 1,
			   sizeof(struct deltacloud_create_parameter));
  if (internal_params == NULL) {
    oom_error();
    return -1;
  }

  pos = copy_parameters(internal_params, params, params_length);
  if (pos < 0)
    /* copy_parameters already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "volume_id",
				   volume_id) < 0)
    /* deltacloud_create_parameter already set the error */
    goto cleanup;

  if (internal_create(api, "storage_snapshots", internal_params, pos, NULL) < 0)
    /* internal_create already set the error */
    goto cleanup;

  ret = 0;

 cleanup:
  free_parameters(internal_params, pos);
  SAFE_FREE(internal_params);

  return ret;
}

/**
 * A function to destroy a storage snapshot.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] storage_snapshot The deltacloud_storage_snapstho structure
 *                             representing the storage snapshot
 * @returns 0 on success, -1 on error
 */
int deltacloud_storage_snapshot_destroy(struct deltacloud_api *api,
					struct deltacloud_storage_snapshot *storage_snapshot)
{
  if (!valid_api(api) || !valid_arg(storage_snapshot))
    return -1;

  return internal_destroy(storage_snapshot->href, api->user, api->password);
}

/**
 * A function to free a deltacloud_storage_snapshot structure initially
 * allocated by deltacloud_get_storage_snapshot_by_id().
 * @param[in] storage_snapshot The deltacloud_storage_snapshot structure
 *                             representing the storage snapshot
 */
void deltacloud_free_storage_snapshot(struct deltacloud_storage_snapshot *storage_snapshot)
{
  if (storage_snapshot == NULL)
    return;

  SAFE_FREE(storage_snapshot->href);
  SAFE_FREE(storage_snapshot->id);
  SAFE_FREE(storage_snapshot->created);
  SAFE_FREE(storage_snapshot->state);
  SAFE_FREE(storage_snapshot->storage_volume_href);
  SAFE_FREE(storage_snapshot->storage_volume_id);
}

/**
 * A function to free a list of deltacloud_storage_snapshot structures initially
 * allocated by deltacloud_get_storage_snapshots().
 * @param[in] storage_snapshots The pointer to the head of the
 *                              deltacloud_storage_snapshot list
 */
void deltacloud_free_storage_snapshot_list(struct deltacloud_storage_snapshot **storage_snapshots)
{
  free_list(storage_snapshots, struct deltacloud_storage_snapshot,
	    deltacloud_free_storage_snapshot);
}
