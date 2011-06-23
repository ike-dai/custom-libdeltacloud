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
#include <unistd.h>
#include "libdeltacloud.h"

static void print_firewall(struct deltacloud_firewall *firewall)
{
  struct deltacloud_firewall_rule *rule;
  struct deltacloud_firewall_rule_source *source;

  fprintf(stderr, "Firewall: %s\n", firewall->name);
  fprintf(stderr, "\tHref: %s\n", firewall->href);
  fprintf(stderr, "\tID: %s\n", firewall->id);
  fprintf(stderr, "\tDescription: %s\n", firewall->description);
  fprintf(stderr, "\tOwner ID: %s\n", firewall->owner_id);

  deltacloud_for_each(rule, firewall->rules) {
    fprintf(stderr, "\tRule: %s\n", rule->id);
    fprintf(stderr, "\t\tAllow Protocol: %s\n", rule->allow_protocol);
    fprintf(stderr, "\t\tFrom port: %s\n", rule->from_port);
    fprintf(stderr, "\t\tTo Port: %s\n", rule->to_port);
    fprintf(stderr, "\t\tDirection: %s\n", rule->direction);

    deltacloud_for_each(source, rule->sources) {
      fprintf(stderr, "\t\tSource: %s\n", source->name);
      fprintf(stderr, "\t\t\tType: %s\n", source->type);
      fprintf(stderr, "\t\t\tOwner: %s\n", source->owner);
      fprintf(stderr, "\t\t\tPrefix: %s\n", source->prefix);
      fprintf(stderr, "\t\t\tAddress: %s\n", source->address);
      fprintf(stderr, "\t\t\tFamily: %s\n", source->family);
    }
  }
}

static void print_firewall_list(struct deltacloud_firewall *firewalls)
{
  struct deltacloud_firewall *firewall;

  deltacloud_for_each(firewall, firewalls)
    print_firewall(firewall);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_api zeroapi;
  struct deltacloud_firewall *firewalls = NULL;
  struct deltacloud_firewall firewall;
  struct deltacloud_firewall_rule *rule;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to initialize deltacloud: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }

  memset(&zeroapi, 0, sizeof(struct deltacloud_api));

  /* test out deltacloud_supports_firewalls */
  if (deltacloud_supports_firewalls(NULL) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_firewalls to fail with NULL api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_firewalls(&zeroapi) >= 0) {
    fprintf(stderr, "Expected deltacloud_supports_firewalls to fail with uninitialized api, but succeeded\n");
    goto cleanup;
  }

  if (deltacloud_supports_firewalls(&api)) {

    /* test out deltacloud_get_firewalls */
    if (deltacloud_get_firewalls(NULL, &firewalls) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_firewalls to fail with NULL api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_firewalls(&api, NULL) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_firewalls to fail with NULL firewalls, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_firewalls(&zeroapi, &firewalls) >= 0) {
      fprintf(stderr, "Expected deltacloud_get_firewalls to fail with unintialized api, but succeeded\n");
      goto cleanup;
    }

    if (deltacloud_get_firewalls(&api, &firewalls) < 0) {
      fprintf(stderr, "Failed to get firewalls: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }
    print_firewall_list(firewalls);

    deltacloud_free_firewall_list(&firewalls);

    if (deltacloud_create_firewall(&api, "chrisfw", "my fw", NULL, 0) < 0) {
      fprintf(stderr, "Failed to create firewall: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    if (deltacloud_get_firewall_by_id(&api, "chrisfw", &firewall) < 0) {
      fprintf(stderr, "Failed to get the firewall named chrisfw: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    if (deltacloud_firewall_create_rule(&api, &firewall, "tcp", "22", "22",
					"0.0.0.0/0", NULL, 0) < 0) {
      fprintf(stderr, "Failed to create a new firewall rule: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    /* now refresh the firewall information */
    if (deltacloud_get_firewall_by_id(&api, "chrisfw", &firewall) < 0) {
      fprintf(stderr, "Failed to get the firewall named chrisfw: %s\n",
	      deltacloud_get_last_error_string());
      goto cleanup;
    }

    deltacloud_for_each(rule, firewall.rules) {
      fprintf(stderr, "Rule %s\n", rule->id);
      if (deltacloud_firewall_delete_rule(&api, rule) < 0) {
	fprintf(stderr, "Failed to delete firewall rule (ignoring): %s\n",
		deltacloud_get_last_error_string());
      }
    }

    deltacloud_firewall_destroy(&api, &firewall);
  }

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
