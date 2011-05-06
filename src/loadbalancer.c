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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "loadbalancer.h"

void free_lb_instance(struct deltacloud_loadbalancer_instance *instance)
{
  SAFE_FREE(instance->href);
  SAFE_FREE(instance->id);
  free_link_list(&instance->links);
}

int add_to_lb_instance_list(struct deltacloud_loadbalancer_instance **instances,
			    struct deltacloud_loadbalancer_instance *instance)
{
  struct deltacloud_loadbalancer_instance *oneinst;

  oneinst = calloc(1, sizeof(struct deltacloud_loadbalancer_instance));
  if (oneinst == NULL)
    return -1;

  if (strdup_or_null(&oneinst->href, instance->href) < 0)
    goto error;
  if (strdup_or_null(&oneinst->id, instance->id) < 0)
    goto error;
  if (copy_link_list(&oneinst->links, &instance->links) < 0)
    goto error;

  add_to_list(instances, struct deltacloud_loadbalancer_instance, oneinst);

  return 0;

 error:
  free_lb_instance(oneinst);
  SAFE_FREE(oneinst);
  return -1;
}

static int copy_lb_instance_list(struct deltacloud_loadbalancer_instance **dst,
				 struct deltacloud_loadbalancer_instance **src)
{
  copy_list(dst, src, struct deltacloud_loadbalancer_instance,
	    add_to_lb_instance_list, free_lb_instance_list);
}

void free_lb_instance_list(struct deltacloud_loadbalancer_instance **instances)
{
  free_list(instances, struct deltacloud_loadbalancer_instance,
	    free_lb_instance);
}

void free_listener(struct deltacloud_loadbalancer_listener *listener)
{
  SAFE_FREE(listener->protocol);
  SAFE_FREE(listener->load_balancer_port);
  SAFE_FREE(listener->instance_port);
}

int add_to_listener_list(struct deltacloud_loadbalancer_listener **listeners,
			 struct deltacloud_loadbalancer_listener *listener)
{
  struct deltacloud_loadbalancer_listener *onelistener;

  onelistener = calloc(1, sizeof(struct deltacloud_loadbalancer_listener));
  if (onelistener == NULL)
    return -1;

  if (strdup_or_null(&onelistener->protocol, listener->protocol) < 0)
    goto error;
  if (strdup_or_null(&onelistener->load_balancer_port,
		     listener->load_balancer_port) < 0)
    goto error;
  if (strdup_or_null(&onelistener->instance_port, listener->instance_port) < 0)
    goto error;

  add_to_list(listeners, struct deltacloud_loadbalancer_listener, onelistener);

  return 0;

 error:
  free_listener(onelistener);
  SAFE_FREE(onelistener);
  return -1;
}

static int copy_listener_list(struct deltacloud_loadbalancer_listener **dst,
			      struct deltacloud_loadbalancer_listener **src)
{
  copy_list(dst, src, struct deltacloud_loadbalancer_listener,
	    add_to_listener_list, free_listener_list);
}

void free_listener_list(struct deltacloud_loadbalancer_listener **listeners)
{
  free_list(listeners, struct deltacloud_loadbalancer_listener, free_listener);
}

void deltacloud_free_loadbalancer(struct deltacloud_loadbalancer *lb)
{
  if (lb == NULL)
    return;

  SAFE_FREE(lb->href);
  SAFE_FREE(lb->id);
  SAFE_FREE(lb->created_at);
  SAFE_FREE(lb->realm_href);
  SAFE_FREE(lb->realm_id);
  free_action_list(&lb->actions);
  free_address_list(&lb->public_addresses);
  free_listener_list(&lb->listeners);
  free_lb_instance_list(&lb->instances);
}

int copy_loadbalancer(struct deltacloud_loadbalancer *dst,
		      struct deltacloud_loadbalancer *src)
{
  /* with a NULL src, we just return success.  A NULL dst is an error */
  if (src == NULL)
    return 0;
  if (dst == NULL)
    return -1;

  memset(dst, 0, sizeof(struct deltacloud_loadbalancer));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->created_at, src->created_at) < 0)
    goto error;
  if (strdup_or_null(&dst->realm_href, src->realm_href) < 0)
    goto error;
  if (strdup_or_null(&dst->realm_id, src->realm_id) < 0)
    goto error;
  if (copy_action_list(&dst->actions, &src->actions) < 0)
    goto error;
  if (copy_address_list(&dst->public_addresses, &src->public_addresses) < 0)
    goto error;
  if (copy_listener_list(&dst->listeners, &src->listeners) < 0)
    goto error;
  if (copy_lb_instance_list(&dst->instances, &src->instances) < 0)
    goto error;

  return 0;

 error:
  deltacloud_free_loadbalancer(dst);
  return -1;
}

int add_to_loadbalancer_list(struct deltacloud_loadbalancer **lbs,
			     struct deltacloud_loadbalancer *lb)
{
  struct deltacloud_loadbalancer *onelb;

  onelb = calloc(1, sizeof(struct deltacloud_loadbalancer));
  if (onelb == NULL)
    return -1;

  if (copy_loadbalancer(onelb, lb) < 0)
    goto error;

  add_to_list(lbs, struct deltacloud_loadbalancer, onelb);

  return 0;

 error:
  deltacloud_free_loadbalancer(onelb);
  SAFE_FREE(onelb);
  return -1;
}

void deltacloud_free_loadbalancer_list(struct deltacloud_loadbalancer **lbs)
{
  free_list(lbs, struct deltacloud_loadbalancer, deltacloud_free_loadbalancer);
}
