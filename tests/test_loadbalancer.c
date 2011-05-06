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
#include <string.h>
#include "libdeltacloud.h"
#include "test_common.h"

static void print_loadbalancer(struct deltacloud_loadbalancer *lb)
{
  struct deltacloud_loadbalancer_listener *listener;
  struct deltacloud_loadbalancer_instance *instance;

  fprintf(stderr, "LoadBalancer %s\n", lb->id);
  fprintf(stderr, "\tHref: %s\n", lb->href);
  fprintf(stderr, "\tCreated At: %s\n", lb->created_at);
  fprintf(stderr, "\tRealm Href: %s\n", lb->realm_href);
  fprintf(stderr, "\tRealm ID: %s\n", lb->realm_id);
  print_action_list(lb->actions);
  print_address_list("Public Addresses", lb->public_addresses);
  fprintf(stderr, "\tListeners:\n");
  deltacloud_for_each(listener, lb->listeners) {
    fprintf(stderr, "\t\tListener protocol: %s, lb_port %s, instance_port %s\n",
	    listener->protocol, listener->load_balancer_port,
	    listener->instance_port);
  }
  fprintf(stderr, "\tInstances:\n");
  deltacloud_for_each(instance, lb->instances) {
    fprintf(stderr, "\t\tInstance: %s\n", instance->id);
    fprintf(stderr, "\t\t\tHref: %s\n", instance->href);
    print_link_list(instance->links);
  }
}

static void print_loadbalancer_list(struct deltacloud_loadbalancer *lbs)
{
  struct deltacloud_loadbalancer *lb;

  deltacloud_for_each(lb, lbs)
    print_loadbalancer(lb);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_loadbalancer *lbs;
  struct deltacloud_loadbalancer lb;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }

  if (deltacloud_supports_loadbalancers(&api)) {
    if (deltacloud_get_loadbalancers(&api, &lbs) < 0) {
      fprintf(stderr, "Failed to get load balancers: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_loadbalancer_list(lbs);

    if (lbs != NULL) {
      if (deltacloud_get_loadbalancer_by_id(&api, lbs->id, &lb) < 0) {
	fprintf(stderr, "Failed to get load balancer by id: %s\n",
		deltacloud_get_last_error_string());
	deltacloud_free_loadbalancer_list(&lbs);
	goto cleanup;
      }
      print_loadbalancer(&lb);
      deltacloud_free_loadbalancer(&lb);
    }

    deltacloud_free_loadbalancer_list(&lbs);

    if (deltacloud_create_loadbalancer(&api, "lb2", "us-east-1a", "HTTP", 80,
				       80, NULL, 0) < 0) {
      fprintf(stderr, "Failed to create load balancer: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    if (deltacloud_get_loadbalancer_by_id(&api, "lb2", &lb) < 0) {
      fprintf(stderr, "Failed to get load balancer by id: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    if (deltacloud_loadbalancer_destroy(&api, &lb) < 0) {
      fprintf(stderr, "Failed to destroy loadbalancer: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_loadbalancer(&lb);
      goto cleanup;
    }
    deltacloud_free_loadbalancer(&lb);
  }
  else
    fprintf(stderr, "Load Balancers not supported\n");

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
