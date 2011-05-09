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
#include <memory.h>
#include "common.h"
#include "loadbalancer.h"

static void free_lb_instance(struct deltacloud_loadbalancer_instance *instance)
{
  SAFE_FREE(instance->href);
  SAFE_FREE(instance->id);
  free_link_list(&instance->links);
}

static void free_lb_instance_list(struct deltacloud_loadbalancer_instance **instances)
{
  free_list(instances, struct deltacloud_loadbalancer_instance,
	    free_lb_instance);
}

static void free_listener(struct deltacloud_loadbalancer_listener *listener)
{
  SAFE_FREE(listener->protocol);
  SAFE_FREE(listener->load_balancer_port);
  SAFE_FREE(listener->instance_port);
}

static void free_listener_list(struct deltacloud_loadbalancer_listener **listeners)
{
  free_list(listeners, struct deltacloud_loadbalancer_listener, free_listener);
}

static int parse_listener_xml(xmlNodePtr root, xmlXPathContextPtr ctxt,
			      struct deltacloud_loadbalancer_listener **listeners)
{
  struct deltacloud_loadbalancer_listener *thislistener;
  xmlNodePtr oldnode, cur;
  int ret = -1;

  *listeners = NULL;

  oldnode = ctxt->node;

  ctxt->node = root;
  cur = root->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "listener")) {

      thislistener = calloc(1, sizeof(struct deltacloud_loadbalancer_listener));
      if (thislistener == NULL) {
	oom_error();
	goto cleanup;
      }

      thislistener->protocol = (char *)xmlGetProp(cur, BAD_CAST "protocol");
      thislistener->load_balancer_port = getXPathString("string(./listener/load_balancer_port)",
							ctxt);
      thislistener->instance_port = getXPathString("string(./listener/instance_port)",
						   ctxt);

      /* add_to_list can't fail */
      add_to_list(listeners, struct deltacloud_loadbalancer_listener,
		  thislistener);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    free_listener_list(listeners);

  return ret;
}

static int parse_lb_instance_xml(xmlNodePtr root, xmlXPathContextPtr ctxt,
				 struct deltacloud_loadbalancer_instance **instances)
{
  struct deltacloud_loadbalancer_instance *thisinst;
  xmlNodePtr oldnode, cur;
  int ret = -1;

  *instances = NULL;

  oldnode = ctxt->node;

  ctxt->node = root;
  cur = root->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "instance")) {

      thisinst = calloc(1, sizeof(struct deltacloud_loadbalancer_instance));
      if (thisinst == NULL) {
	oom_error();
	goto cleanup;
      }

      thisinst->href = (char *)xmlGetProp(cur, BAD_CAST "href");
      thisinst->id = (char *)xmlGetProp(cur, BAD_CAST "id");

      if (parse_link_xml(cur->children, &(thisinst->links)) < 0) {
	free_lb_instance(thisinst);
	SAFE_FREE(thisinst);
	goto cleanup;
      }

      add_to_list(instances, struct deltacloud_loadbalancer_instance, thisinst);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    free_lb_instance_list(instances);

  return ret;
}

static int parse_one_loadbalancer(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				  void *output)
{
  struct deltacloud_loadbalancer *thislb = (struct deltacloud_loadbalancer *)output;
  xmlXPathObjectPtr actionset, pubset, listenerset, instanceset;

  memset(thislb, 0, sizeof(struct deltacloud_loadbalancer));

  thislb->href = (char *)xmlGetProp(cur, BAD_CAST "href");
  thislb->id = (char *)xmlGetProp(cur, BAD_CAST "id");
  thislb->created_at = getXPathString("string(./created_at)", ctxt);
  thislb->realm_href = getXPathString("string(./realm/@href)", ctxt);
  thislb->realm_id = getXPathString("string(./realm/@id)", ctxt);

  actionset = xmlXPathEval(BAD_CAST "./actions", ctxt);
  if (actionset && actionset->type == XPATH_NODESET &&
      actionset->nodesetval && actionset->nodesetval->nodeNr == 1) {
    if (parse_actions_xml(actionset->nodesetval->nodeTab[0],
			  &(thislb->actions)) < 0) {
      deltacloud_free_loadbalancer(thislb);
      xmlXPathFreeObject(actionset);
      return -1;
    }
  }
  xmlXPathFreeObject(actionset);

  pubset = xmlXPathEval(BAD_CAST "./public_addresses", ctxt);
  if (pubset && pubset->type == XPATH_NODESET && pubset->nodesetval &&
      pubset->nodesetval->nodeNr == 1) {
    if (parse_addresses_xml(pubset->nodesetval->nodeTab[0], ctxt,
			    &(thislb->public_addresses)) < 0) {
      deltacloud_free_loadbalancer(thislb);
      xmlXPathFreeObject(pubset);
      return -1;
    }
  }
  xmlXPathFreeObject(pubset);

  listenerset = xmlXPathEval(BAD_CAST "./listeners", ctxt);
  if (listenerset && listenerset->type == XPATH_NODESET &&
      listenerset->nodesetval && listenerset->nodesetval->nodeNr == 1) {
    if (parse_listener_xml(listenerset->nodesetval->nodeTab[0], ctxt,
			   &(thislb->listeners)) < 0) {
      deltacloud_free_loadbalancer(thislb);
      xmlXPathFreeObject(listenerset);
      return -1;
    }
  }
  xmlXPathFreeObject(listenerset);

  instanceset = xmlXPathEval(BAD_CAST "./instances", ctxt);
  if (instanceset && instanceset->type == XPATH_NODESET &&
      instanceset->nodesetval && instanceset->nodesetval->nodeNr == 1) {
    if (parse_lb_instance_xml(instanceset->nodesetval->nodeTab[0], ctxt,
			      &(thislb->instances)) < 0) {
      deltacloud_free_loadbalancer(thislb);
      xmlXPathFreeObject(instanceset);
      return -1;
    }
  }
  xmlXPathFreeObject(instanceset);

  return 0;
}

static int parse_loadbalancer_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				  void **data)
{
  struct deltacloud_loadbalancer **lbs = (struct deltacloud_loadbalancer **)data;
  struct deltacloud_loadbalancer *thislb;
  int ret = -1;
  xmlNodePtr oldnode;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "load_balancer")) {

      ctxt->node = cur;

      thislb = calloc(1, sizeof(struct deltacloud_loadbalancer));
      if (thislb == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_loadbalancer(cur, ctxt, thislb) < 0) {
	/* parse_one_loadbalancer is expected to have set its own error */
	SAFE_FREE(thislb);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(lbs, struct deltacloud_loadbalancer, thislb);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_loadbalancer_list(lbs);

  return ret;
}

static int prepare_int_parameter(struct deltacloud_create_parameter *param,
				 const char *name, int value)
{
  param->name = strdup(name);
  if (param->name == NULL) {
    oom_error();
    return -1;
  }

  if (asprintf(&param->value, "%d", value) < 0) {
    SAFE_FREE(param->name);
    oom_error();
    return -1;
  }

  return 0;
}

int deltacloud_get_loadbalancers(struct deltacloud_api *api,
				 struct deltacloud_loadbalancer **balancers)
{
  return internal_get(api, "load_balancers", "load_balancers",
		      parse_loadbalancer_xml, (void **)balancers);
}

int deltacloud_get_loadbalancer_by_id(struct deltacloud_api *api,
				      const char *id,
				      struct deltacloud_loadbalancer *balancer)
{
  return internal_get_by_id(api, id, "load_balancers", "load_balancer",
			    parse_one_loadbalancer, balancer);
}

int deltacloud_create_loadbalancer(struct deltacloud_api *api, const char *name,
				   const char *realm_id, const char *protocol,
				   int balancer_port, int instance_port,
				   struct deltacloud_create_parameter *params,
				   int params_length)
{
  struct deltacloud_create_parameter *internal_params;
  int ret = -1;
  int pos;

  if (!valid_api(api) || !valid_arg(name) || !valid_arg(realm_id)
      || !valid_arg(protocol))
    return -1;
  if (balancer_port < 0 || balancer_port > 65536) {
    invalid_argument_error("balancer_port must be between 0 and 65536");
    return -1;
  }
  if (instance_port < 0 || instance_port > 65536) {
    invalid_argument_error("instance_port must be between 0 and 65536");
    return -1;
  }

  if (params_length < 0) {
    invalid_argument_error("params_length must be >= 0");
    return -1;
  }

  internal_params = calloc(params_length + 5,
			   sizeof(struct deltacloud_create_parameter));
  if (internal_params == NULL) {
    oom_error();
    return -1;
  }

  pos = copy_parameters(internal_params, params, params_length);
  if (pos < 0)
    /* copy_parameters already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "name", name) < 0 ||
      deltacloud_prepare_parameter(&internal_params[pos++], "realm_id", realm_id) < 0 ||
      deltacloud_prepare_parameter(&internal_params[pos++], "listener_protocol", protocol) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (prepare_int_parameter(&internal_params[pos++], "listener_balancer_port",
			    balancer_port) < 0)
    /* prepare_int_parameter already set the error */
    goto cleanup;

  if (prepare_int_parameter(&internal_params[pos++], "listener_instance_port",
			    instance_port) < 0)
    /* prepare_int_parameter already set the error */
    goto cleanup;

  ret = 0;

 cleanup:
  free_parameters(internal_params, pos);
  SAFE_FREE(internal_params);

  return ret;
}

int deltacloud_loadbalancer_destroy(struct deltacloud_api *api,
				    struct deltacloud_loadbalancer *balancer)
{
  if (!valid_api(api) || !valid_arg(balancer))
    return -1;

  return internal_destroy(balancer->href, api->user, api->password);
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

void deltacloud_free_loadbalancer_list(struct deltacloud_loadbalancer **lbs)
{
  free_list(lbs, struct deltacloud_loadbalancer, deltacloud_free_loadbalancer);
}
