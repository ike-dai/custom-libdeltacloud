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

#ifndef LIBDELTACLOUD_FIREWALL_H
#define LIBDELTACLOUD_FIREWALL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A structure representing a single source in a firewall rule.
 */
struct deltacloud_firewall_rule_source {
  char *type; /**< The type of this firewall rule source */
  char *name; /**< The name of this firewall rule source */
  char *owner; /**< The owner of this firewall rule source */
  char *prefix; /**< The prefix of this firewall rule source */
  char *address; /**< The allowed source address of this firewall rule source */
  char *family; /**< The allowed protocol family of this firewall rule source */

  struct deltacloud_firewall_rule_source *next;
};

/**
 * A structure representing a single firewall rule.
 */
struct deltacloud_firewall_rule {
  char *href; /**< The full URL to this firewall rule */
  char *id; /**< The ID of this firewall rule */
  char *allow_protocol; /**< The allowed protocols of this firewall rule */
  char *from_port; /**< The start of the port range of this firewall rule */
  char *to_port; /**< The end of the port range of this firewall rule */
  char *direction; /**< The allowed direction of this firewall rule */

  struct deltacloud_firewall_rule_source *sources;

  struct deltacloud_firewall_rule *next;
};

/**
 * A structure representing a single firewall.
 */
struct deltacloud_firewall {
  char *href; /**< The full URL to this firewall */
  char *id; /**< The ID of this firewall */
  char *name; /**< The name of this firewall */
  char *description; /**< The description of this firewall */
  char *owner_id; /**< The owner ID of this firewall */

  struct deltacloud_firewall_rule *rules;

  struct deltacloud_firewall *next;
};

#define deltacloud_supports_firewalls(api) deltacloud_has_link(api, "firewalls")
int deltacloud_get_firewalls(struct deltacloud_api *api,
			     struct deltacloud_firewall **firewalls);
int deltacloud_get_firewall_by_id(struct deltacloud_api *api, const char *id,
				  struct deltacloud_firewall *firewall);
int deltacloud_create_firewall(struct deltacloud_api *api, const char *name,
			       const char *description,
			       struct deltacloud_create_parameter *params,
			       int params_length);
int deltacloud_firewall_create_rule(struct deltacloud_api *api,
				    struct deltacloud_firewall *firewall,
				    const char *protocol, const char *from_port,
				    const char *to_port,
				    const char *ipaddresses,
				    struct deltacloud_create_parameter *params,
				    int params_length);
int deltacloud_firewall_delete_rule(struct deltacloud_api *api,
				    struct deltacloud_firewall_rule *rule);
int deltacloud_firewall_destroy(struct deltacloud_api *api,
				struct deltacloud_firewall *firewall);
void deltacloud_free_firewall(struct deltacloud_firewall *firewall);
void deltacloud_free_firewall_list(struct deltacloud_firewall **firewalls);

#ifdef __cplusplus
}
#endif

#endif
