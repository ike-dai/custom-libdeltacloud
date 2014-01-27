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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "firewall.h"

/** @file */

static void free_firewall_rule_source(struct deltacloud_firewall_rule_source *source)
{
  SAFE_FREE(source->type);
  SAFE_FREE(source->name);
  SAFE_FREE(source->owner);
  SAFE_FREE(source->prefix);
  SAFE_FREE(source->address);
  SAFE_FREE(source->family);
}

static void free_firewall_rule(struct deltacloud_firewall_rule *rule)
{
  SAFE_FREE(rule->href);
  SAFE_FREE(rule->id);
  SAFE_FREE(rule->allow_protocol);
  SAFE_FREE(rule->from_port);
  SAFE_FREE(rule->to_port);
  SAFE_FREE(rule->direction);
  free_list(&rule->sources, struct deltacloud_firewall_rule_source,
	    free_firewall_rule_source);
}

static int parse_one_rule(xmlNodePtr cur, xmlXPathContextPtr ctxt, void *output)
{
  struct deltacloud_firewall_rule *thisrule = (struct deltacloud_firewall_rule *)output;
  struct deltacloud_firewall_rule_source *thissource;
  xmlNodePtr source_cur, oldnode, asource_cur;
  int ret = -1;

  oldnode = ctxt->node;

  memset(thisrule, 0, sizeof(struct deltacloud_firewall_rule));

  thisrule->href = getXPathString("string(./@href)", ctxt);
  thisrule->id = getXPathString("string(./@id)", ctxt);
  thisrule->allow_protocol = getXPathString("string(./allow_protocol)", ctxt);
  thisrule->from_port = getXPathString("string(./port_from)", ctxt);
  thisrule->to_port = getXPathString("string(./port_to)", ctxt);
  thisrule->direction = getXPathString("string(./direction)", ctxt);

  for (source_cur = cur->children; source_cur != NULL;
       source_cur = source_cur->next) {
    if (source_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)source_cur->name, "sources")) {

      for (asource_cur = source_cur->children; asource_cur != NULL;
	   asource_cur = asource_cur->next) {
	if (asource_cur->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)asource_cur->name, "source")) {

	  ctxt->node = asource_cur;

	  thissource = calloc(1,
			      sizeof(struct deltacloud_firewall_rule_source));
	  if (thissource == NULL) {
	    oom_error();
	    goto cleanup;
	  }

	  thissource->type = getXPathString("string(./@type)", ctxt);
	  thissource->name = getXPathString("string(./@name)", ctxt);
	  thissource->owner = getXPathString("string(./@owner)", ctxt);
	  thissource->prefix = getXPathString("string(./@prefix)", ctxt);
	  thissource->address = getXPathString("string(./@address)", ctxt);
	  thissource->family = getXPathString("string(./@family)", ctxt);

	  /* add_to_list can't fail */
	  add_to_list(&thisrule->sources,
		      struct deltacloud_firewall_rule_source, thissource);
	}
      }
    }
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;

  return ret;
}

static int parse_one_firewall(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			      void *output)
{
  struct deltacloud_firewall *thisfirewall = (struct deltacloud_firewall *)output;
  struct deltacloud_firewall_rule *thisrule;
  xmlNodePtr rule_cur, arule_cur, oldnode;
  int ret = -1;

  oldnode = ctxt->node;

  memset(thisfirewall, 0, sizeof(struct deltacloud_firewall));

  thisfirewall->href = getXPathString("string(./@href)", ctxt);
  thisfirewall->id = getXPathString("string(./@id)", ctxt);
  thisfirewall->name = getXPathString("string(./name)", ctxt);
  thisfirewall->description = getXPathString("string(./description)", ctxt);
  thisfirewall->owner_id = getXPathString("string(./owner_id)", ctxt);

  for (rule_cur = cur->children; rule_cur != NULL; rule_cur = rule_cur->next) {
    if (rule_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)rule_cur->name, "rules")) {

      for (arule_cur = rule_cur->children; arule_cur != NULL;
	   arule_cur = arule_cur->next) {
	if (arule_cur->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)arule_cur->name, "rule")) {

	  ctxt->node = arule_cur;

	  thisrule = calloc(1, sizeof(struct deltacloud_firewall_rule));
	  if (thisrule == NULL) {
	    oom_error();
	    goto cleanup;
	  }

	  if (parse_one_rule(arule_cur, ctxt, thisrule) < 0) {
	    /* parse_one_rule already set the error */
	    SAFE_FREE(thisrule);
	    goto cleanup;
	  }

	  /* add_to_list can't fail */
	  add_to_list(&thisfirewall->rules, struct deltacloud_firewall_rule,
		      thisrule);
	}
      }
    }
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_firewall(thisfirewall);

  return ret;
}

static int parse_firewall_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			      void **data)
{
  struct deltacloud_firewall **firewalls = (struct deltacloud_firewall **)data;
  struct deltacloud_firewall *thisfirewall;
  xmlNodePtr oldnode;
  int ret = -1;

  oldnode = ctxt->node;

  for ( ; cur != NULL; cur = cur->next) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "firewall")) {

      ctxt->node = cur;

      thisfirewall = calloc(1, sizeof(struct deltacloud_firewall));
      if (thisfirewall == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_firewall(cur, ctxt, thisfirewall) < 0) {
	/* parse_one_firewall is expected to have set its own error */
	SAFE_FREE(thisfirewall);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(firewalls, struct deltacloud_firewall, thisfirewall);
    }
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_firewall_list(firewalls);
  ctxt->node = oldnode;

  return ret;
}

/**
 * A function to get a linked list of all of the firewalls.  The caller
 * is expected to free the list using deltacloud_free_firewall_list().
 * @param[in] api The deltacloud_api structure representing this connection
 * @param[out] firewalls A pointer to the deltacloud_firewall structure to hold
 *                       the list of firewalls
 * @returns 0 on success, -1 on error
 */
int deltacloud_get_firewalls(struct deltacloud_api *api,
			     struct deltacloud_firewall **firewalls)
{
  return internal_get(api, "firewalls", "firewalls", parse_firewall_xml,
		      (void **)firewalls);
}

/**
 * A function to look up a particular firewall by id.  The caller is expected
 * to free the deltacloud_firewall structure using deltacloud_free_firewall().
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] id The firewall ID to look for
 * @param[out] firewall The deltacloud_firewall structure to fill in if the ID
 *                      is found
 * @returns 0 on success, -1 if the firewall cannot be found or on error
 */
int deltacloud_get_firewall_by_id(struct deltacloud_api *api, const char *id,
				  struct deltacloud_firewall *firewall)
{
  return internal_get_by_id(api, id, "firewalls", "firewall",
			    parse_one_firewall, firewall);
}

/**
 * A function to create a new firewall.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] name The name to assign to the new firewall
 * @param[in] description The description to assign to the new firewall
 * @param[in] params An array of deltacloud_create_parameter structures that
 *                   represent any optional parameters to pass into the
 *                   create call
 * @param[in] params_length An integer describing the length of the params
 *                          array
 * @returns 0 on success, -1 on error
 */
int deltacloud_create_firewall(struct deltacloud_api *api, const char *name,
			       const char *description,
			       struct deltacloud_create_parameter *params,
			       int params_length)
{
  struct deltacloud_create_parameter *internal_params;
  int ret = -1;
  int pos;

  if (!valid_api(api) || !valid_arg(name) || !valid_arg(description))
    return -1;

  if (params_length < 0) {
    invalid_argument_error("params_length must be >= 0");
    return -1;
  }

  internal_params = calloc(params_length + 2,
			   sizeof(struct deltacloud_create_parameter));
  if (internal_params == NULL) {
    oom_error();
    return -1;
  }

  pos = copy_parameters(internal_params, params, params_length);
  if (pos < 0)
    /* copy_parameters already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "name", name) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "description",
				   description) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (internal_create(api, "firewalls", internal_params, pos, NULL, NULL) < 0)
    /* internal_create already set the error */
    goto cleanup;

  ret = 0;

 cleanup:
  free_parameters(internal_params, pos);
  SAFE_FREE(internal_params);

  return ret;
}

/**
 * A function to create a new rule for an existing firewall.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] firewall The deltacloud_firewall structure representing the
                       firewall
 * @param[in] protocol The protocol type for the new firewall rule
 * @param[in] from_port The start of the port range for the new firewall rule
 * @param[in] to_port The end of the port range for the new firewall rule
 * @param[in] ipaddresses A comma separated list of IPv4 CIDR addresses to allow
 *                        (e.g. 0.0.0.0/0)
 * @param[in] params An array of deltacloud_create_parameter structures that
 *                   represent any optional parameters to pass into the
 *                   create call
 * @param[in] params_length An integer describing the length of the params
 *                          array
 * @returns 0 on success, -1 on error
 */
int deltacloud_firewall_create_rule(struct deltacloud_api *api,
				    struct deltacloud_firewall *firewall,
				    const char *protocol, const char *from_port,
				    const char *to_port,
				    const char *ipaddresses,
				    struct deltacloud_create_parameter *params,
				    int params_length)
{
  struct deltacloud_create_parameter *internal_params;
  char *href;
  int pos;
  int ret = -1;
  int rc;

  if (!valid_api(api) || !valid_arg(firewall) || !valid_arg(protocol) ||
      !valid_arg(from_port) || !valid_arg(to_port) || !valid_arg(ipaddresses))
    return -1;

  if (params_length < 0) {
    invalid_argument_error("params_length must be >= 0");
    return -1;
  }

  internal_params = calloc(params_length + 4,
			   sizeof(struct deltacloud_create_parameter));
  if (internal_params == NULL) {
    oom_error();
    return -1;
  }

  pos = copy_parameters(internal_params, params, params_length);
  if (pos < 0)
    /* copy_parameters already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "protocol",
				   protocol) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "port_from",
				   from_port) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "port_to",
				   to_port) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "ip-address",
				   ipaddresses) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (asprintf(&href, "%s/rules", firewall->href) < 0) {
    oom_error();
    goto cleanup;
  }

  rc = internal_post(api, href, internal_params, pos, NULL, NULL);
  SAFE_FREE(href);
  if (rc < 0)
    /* internal_post already set the error */
    goto cleanup;

  ret = 0;

 cleanup:
  free_parameters(internal_params, pos);
  SAFE_FREE(internal_params);

  return ret;
}

/**
 * A function to delete a rule from a firewall.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] rule The deltacloud_firewall_rule structure representing the
                   rule to be deleted
 * @returns 0 on success, -1 on error
 */
int deltacloud_firewall_delete_rule(struct deltacloud_api *api,
				    struct deltacloud_firewall_rule *rule)
{
  if (!valid_api(api) || !valid_arg(rule))
    return -1;

  return internal_destroy(rule->href, api->user, api->password, api->driver, api->provider);
}

/**
 * A function to destroy a firewall.
 * @param[in] api The deltacloud_api structure representing the connection
 * @param[in] firewall The deltacloud_firewall structure representing the
                       firewall
 * @returns 0 on success, -1 on error
 */
int deltacloud_firewall_destroy(struct deltacloud_api *api,
				struct deltacloud_firewall *firewall)
{
  if (!valid_api(api) || !valid_arg(firewall))
    return -1;

  return internal_destroy(firewall->href, api->user, api->password, api->driver, api->provider);
}

/**
 * A function to free a deltacloud_firewall structure initially allocated
 * by deltacloud_get_firewall_by_id().
 * @param[in] firewall The deltacloud_firewall structure representing the
 *                     firewall
 */
void deltacloud_free_firewall(struct deltacloud_firewall *firewall)
{
  if (firewall == NULL)
    return;

  SAFE_FREE(firewall->href);
  SAFE_FREE(firewall->id);
  SAFE_FREE(firewall->name);
  SAFE_FREE(firewall->description);
  SAFE_FREE(firewall->owner_id);
  free_list(&firewall->rules, struct deltacloud_firewall_rule,
	    free_firewall_rule);
}

/**
 * A function to free a list of deltacloud_firewall structures initially
 * allocated by deltacloud_get_firewall().
 * @param[in] firewalls The pointer to the head of the deltacloud_firewall list
 */
void deltacloud_free_firewall_list(struct deltacloud_firewall **firewalls)
{
  free_list(firewalls, struct deltacloud_firewall, deltacloud_free_firewall);
}
