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
#include <memory.h>
#include "common.h"
#include "instance.h"

void free_address(struct deltacloud_address *addr)
{
  SAFE_FREE(addr->address);
}

int add_to_address_list(struct deltacloud_address **addresses,
			struct deltacloud_address *address)
{
  struct deltacloud_address *oneaddress;

  oneaddress = calloc(1, sizeof(struct deltacloud_address));
  if (oneaddress == NULL)
    return -1;

  if (strdup_or_null(&oneaddress->address, address->address) < 0)
    goto error;

  add_to_list(addresses, struct deltacloud_address, oneaddress);

  return 0;

 error:
  free_address(oneaddress);
  SAFE_FREE(oneaddress);
  return -1;
}

static int copy_address_list(struct deltacloud_address **dst,
			     struct deltacloud_address **src)
{
  copy_list(dst, src, struct deltacloud_address, add_to_address_list,
	    free_address_list);
}

void free_address_list(struct deltacloud_address **addresses)
{
  free_list(addresses, struct deltacloud_address, free_address);
}

void free_action(struct deltacloud_action *action)
{
  SAFE_FREE(action->rel);
  SAFE_FREE(action->href);
  SAFE_FREE(action->method);
}

int add_to_action_list(struct deltacloud_action **actions,
		       struct deltacloud_action *action)
{
  struct deltacloud_action *oneaction;

  oneaction = calloc(1, sizeof(struct deltacloud_action));
  if (oneaction == NULL)
    return -1;

  if (strdup_or_null(&oneaction->rel, action->rel) < 0)
    goto error;
  if (strdup_or_null(&oneaction->href, action->href) < 0)
    goto error;
  if (strdup_or_null(&oneaction->method, action->method) < 0)
    goto error;

  add_to_list(actions, struct deltacloud_action, oneaction);

  return 0;

 error:
  free_action(oneaction);
  SAFE_FREE(oneaction);
  return -1;
}

static int copy_action_list(struct deltacloud_action **dst,
			    struct deltacloud_action **src)
{
  copy_list(dst, src, struct deltacloud_action, add_to_action_list,
	    free_action_list);
}

void free_action_list(struct deltacloud_action **actions)
{
  free_list(actions, struct deltacloud_action, free_action);
}

int copy_instance(struct deltacloud_instance *dst,
		  struct deltacloud_instance *src)
{
  /* with a NULL src, we just return success.  A NULL dst is an error */
  if (src == NULL)
    return 0;
  if (dst == NULL)
    return -1;

  memset(dst, 0, sizeof(struct deltacloud_instance));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->name, src->name) < 0)
    goto error;
  if (strdup_or_null(&dst->owner_id, src->owner_id) < 0)
    goto error;
  if (strdup_or_null(&dst->image_id, src->image_id) < 0)
    goto error;
  if (strdup_or_null(&dst->image_href, src->image_href) < 0)
    goto error;
  if (strdup_or_null(&dst->realm_id, src->realm_id) < 0)
    goto error;
  if (strdup_or_null(&dst->realm_href, src->realm_href) < 0)
    goto error;
  if (strdup_or_null(&dst->state, src->state) < 0)
    goto error;
  if (copy_hardware_profile(&dst->hwp, &src->hwp) < 0)
    goto error;
  if (copy_action_list(&dst->actions, &src->actions) < 0)
    goto error;
  if (copy_address_list(&dst->public_addresses, &src->public_addresses) < 0)
    goto error;
  if (copy_address_list(&dst->private_addresses, &src->private_addresses) < 0)
    goto error;
  dst->next = NULL;

  return 0;

 error:
  deltacloud_free_instance(dst);
  return -1;
}

int add_to_instance_list(struct deltacloud_instance **instances,
			 struct deltacloud_instance *instance)
{
  struct deltacloud_instance *oneinstance;

  oneinstance = calloc(1, sizeof(struct deltacloud_instance));
  if (oneinstance == NULL)
    return -1;

  if (copy_instance(oneinstance, instance) < 0)
    goto error;

  add_to_list(instances, struct deltacloud_instance, oneinstance);

  return 0;

 error:
  deltacloud_free_instance(oneinstance);
  SAFE_FREE(oneinstance);
  return -1;
}

void deltacloud_free_instance(struct deltacloud_instance *instance)
{
  if (instance == NULL)
    return;

  SAFE_FREE(instance->href);
  SAFE_FREE(instance->id);
  SAFE_FREE(instance->name);
  SAFE_FREE(instance->owner_id);
  SAFE_FREE(instance->image_id);
  SAFE_FREE(instance->image_href);
  SAFE_FREE(instance->realm_id);
  SAFE_FREE(instance->realm_href);
  SAFE_FREE(instance->state);
  deltacloud_free_hardware_profile(&instance->hwp);
  free_action_list(&instance->actions);
  free_address_list(&instance->public_addresses);
  free_address_list(&instance->private_addresses);
}

void deltacloud_free_instance_list(struct deltacloud_instance **instances)
{
  free_list(instances, struct deltacloud_instance, deltacloud_free_instance);
}
