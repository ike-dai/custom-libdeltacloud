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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include "common.h"
#include "geturl.h"
#include "link.h"
#include "instance.h"
#include "realm.h"
#include "flavor.h"
#include "image.h"
#include "instance_state.h"
#include "storage_volume.h"
#include "storage_snapshot.h"

struct deltacloud_api {
  char *url;
  char *user;
  char *password;

  struct link *links;
};

typedef int (*parse_xml_callback)(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data);

static int parse_xml(const char *xml_string, const char *name, void **data,
		     parse_xml_callback cb)
{
  xmlDocPtr xml;
  xmlNodePtr root;
  xmlXPathContextPtr ctxt = NULL;
  int ret = -1;

  xml = xmlReadDoc(BAD_CAST xml_string, name, NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    fprintf(stderr, "Failed to parse XML\n");
    return -1;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    fprintf(stderr, "Failed to get the root element\n");
    goto cleanup;
  }

  if (STRNEQ((const char *)root->name, name)) {
    fprintf(stderr, "Root element was not '%s'\n", name);
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  if (cb(root->children, ctxt, data) < 0) {
    fprintf(stderr, "Could not parse XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);

  return ret;
}

static int parse_api_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data)
{
  struct link **links = (struct link **)data;
  char *href = NULL, *rel = NULL;
  int listret;
  int ret = -1;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "link")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      rel = (char *)xmlGetProp(cur, BAD_CAST "rel");

      if (href == NULL) {
	fprintf(stderr, "Did not see href xml property\n");
	goto cleanup;
      }
      if (rel == NULL) {
	fprintf(stderr, "Did not see rel xml property\n");
	MY_FREE(href);
	goto cleanup;
      }

      listret = add_to_link_list(links, href, rel);
      MY_FREE(href);
      MY_FREE(rel);
      if (listret < 0) {
	fprintf(stderr, "Failed to add to link list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_link_list(links);

  return ret;
}

static int get_links(struct deltacloud_api *api)
{
  char *data;
  int ret = -1;

  data = get_url(api->url, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Failed to get API data from url %s\n", api->url);
    return -1;
  }

  api->links = NULL;
  if (parse_xml(data, "api", (void **)&api->links, parse_api_xml) < 0) {
    fprintf(stderr, "Failed to parse the API XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);
  return ret;
}

static char *getXPathString(const char *xpath, xmlXPathContextPtr ctxt)
{
  xmlXPathObjectPtr obj;
  xmlNodePtr relnode;
  char *ret;

  if ((ctxt == NULL) || (xpath == NULL)) {
    fprintf(stderr, "Invalid parameter to virXPathString()");
    return NULL;
  }

  relnode = ctxt->node;
  obj = xmlXPathEval(BAD_CAST xpath, ctxt);
  ctxt->node = relnode;
  if ((obj == NULL) || (obj->type != XPATH_STRING) ||
      (obj->stringval == NULL) || (obj->stringval[0] == 0)) {
    xmlXPathFreeObject(obj);
    return NULL;
  }
  ret = strdup((char *) obj->stringval);
  xmlXPathFreeObject(obj);
  if (ret == NULL)
    fprintf(stderr, "Failed to allocate memory\n");

  return ret;
}

static struct address *parse_addresses_xml(xmlNodePtr instance,
					   xmlXPathContextPtr ctxt)
{
  xmlNodePtr oldnode, cur;
  struct address *addresses = NULL;
  char *address = NULL;
  int failed = 1;
  int listret;

  oldnode = ctxt->node;

  ctxt->node = instance;
  cur = instance->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "address")) {
      address = getXPathString("string(./address)", ctxt);
      if (address == NULL) {
	fprintf(stderr, "Could find XML for address\n");
	goto cleanup;
      }
      listret = add_to_address_list(&addresses, address);
      MY_FREE(address);
      if (listret < 0) {
	fprintf(stderr, "Failed to add address to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  failed = 0;

 cleanup:
  ctxt->node = oldnode;

  if (failed)
    free_address_list(&addresses);

  return addresses;
}

static struct action *parse_actions_xml(xmlNodePtr instance)
{
  xmlNodePtr cur;
  struct action *actions = NULL;
  char *rel = NULL, *href = NULL;
  int failed = 1;
  int listret;

  cur = instance->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "link")) {
      rel = (char *)xmlGetProp(cur, BAD_CAST "rel");
      href = (char *)xmlGetProp(cur, BAD_CAST "href");

      listret = add_to_action_list(&actions, rel, href);
      MY_FREE(href);
      MY_FREE(rel);
      if (listret < 0) {
	fprintf(stderr, "Could not add new action to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  failed = 0;

 cleanup:
  if (failed)
    free_action_list(&actions);

  return actions;
}

static int parse_instance_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			      void **data)
{
  struct instance **instances = (struct instance **)data;
  xmlNodePtr oldnode, instance_cur;
  char *id = NULL, *name = NULL, *owner_id = NULL, *image_href = NULL;
  char *flavor_href = NULL, *realm_href = NULL, *state = NULL;
  struct action *actions = NULL;
  struct address *public_addresses = NULL;
  struct address *private_addresses = NULL;
  int listret;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "instance")) {

      ctxt->node = cur;
      instance_cur = cur->children;
      while (instance_cur != NULL) {
	if (instance_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)instance_cur->name, "id"))
	    id = getXPathString("string(./id)", ctxt);
	  else if (STREQ((const char *)instance_cur->name, "name"))
	    name = getXPathString("string(./name)", ctxt);
	  else if (STREQ((const char *)instance_cur->name, "owner_id"))
	    owner_id = getXPathString("string(./owner_id)", ctxt);
	  else if (STREQ((const char *)instance_cur->name, "image"))
	    image_href = (char *)xmlGetProp(cur, BAD_CAST "href");
	  else if (STREQ((const char *)instance_cur->name, "flavor"))
	    flavor_href = (char *)xmlGetProp(cur, BAD_CAST "href");
	  else if (STREQ((const char *)instance_cur->name, "realm"))
	    realm_href = (char *)xmlGetProp(cur, BAD_CAST "href");
	  else if (STREQ((const char *)instance_cur->name, "state"))
	    state = getXPathString("string(./state)", ctxt);
	  else if (STREQ((const char *)instance_cur->name, "actions"))
	    actions = parse_actions_xml(instance_cur);
	  else if (STREQ((const char *)instance_cur->name, "public-addresses"))
	    public_addresses = parse_addresses_xml(instance_cur, ctxt);
	  else if (STREQ((const char *)instance_cur->name, "private-addresses"))
	    private_addresses = parse_addresses_xml(instance_cur, ctxt);
	}

	instance_cur = instance_cur->next;
      }
      listret = add_to_instance_list(instances, id, name, owner_id, image_href,
				     flavor_href, realm_href, state, actions,
				     public_addresses, private_addresses);
      MY_FREE(id);
      MY_FREE(name);
      MY_FREE(owner_id);
      MY_FREE(image_href);
      MY_FREE(flavor_href);
      MY_FREE(realm_href);
      MY_FREE(state);
      free_address_list(&public_addresses);
      free_address_list(&private_addresses);
      free_action_list(&actions);
      if (listret < 0) {
	fprintf(stderr, "Failed to add new instance to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_instance_list(instances);
  ctxt->node = oldnode;

  return ret;
}

static int get_instances(struct deltacloud_api *api,
			 struct instance **instances)
{
  struct link *thislink;
  char *data;
  int ret = -1;

  thislink = find_by_rel_in_link_list(&api->links, "instances");
  if (thislink == NULL) {
    fprintf(stderr, "Could not find the link for 'instances'\n");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML for instances from url %s\n",
	    thislink->href);
    return -1;
  }

  *instances = NULL;
  if (parse_xml(data, "instances", (void **)instances, parse_instance_xml) < 0) {
    fprintf(stderr, "Failed to parse the instances XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int get_instance_by_id(struct deltacloud_api *api, const char *id,
			      struct instance *instance)
{
  char *url, *data;
  struct instance *tmpinstance = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;

  if (asprintf(&url, "%s/instances/%s", api->url, id) < 0) {
    fprintf(stderr, "Failed to allocate memory for url\n");
    return -1;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML data from url %s\n", url);
    goto cleanup;
  }

  xml = xmlReadDoc(BAD_CAST data, "instance.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    fprintf(stderr, "Failed to parse XML\n");
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    fprintf(stderr, "Failed to get the root element\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  if (parse_instance_xml(root, ctxt, (void **)&tmpinstance) < 0) {
    fprintf(stderr, "Could not parse 'instance' XML\n");
    goto cleanup;
  }

  if (copy_instance(instance, tmpinstance) < 0) {
    fprintf(stderr, "Could not copy instance structure\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  free_instance_list(&tmpinstance);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  MY_FREE(data);
  MY_FREE(url);

  return ret;
}

static int parse_realm_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			   void **data)
{
  struct realm **realms = (struct realm **)data;
  xmlNodePtr oldnode, realm_cur;
  int ret = -1;
  char *href = NULL, *id = NULL, *name = NULL, *state = NULL, *limit = NULL;
  int listret;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "realm")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");

      ctxt->node = cur;
      realm_cur = cur->children;
      while (realm_cur != NULL) {
	if (realm_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)realm_cur->name, "id"))
	    id = getXPathString("string(./id)", ctxt);
	  else if (STREQ((const char *)realm_cur->name, "name"))
	    name = getXPathString("string(./name)", ctxt);
	  else if (STREQ((const char *)realm_cur->name, "state"))
	    state = getXPathString("string(./state)", ctxt);
	  else if (STREQ((const char *)realm_cur->name, "limit"))
	    limit = getXPathString("string(./limit)", ctxt);
	}
	realm_cur = realm_cur->next;
      }
      listret = add_to_realm_list(realms, href, id, name, state, limit);
      MY_FREE(href);
      MY_FREE(id);
      MY_FREE(name);
      MY_FREE(state);
      MY_FREE(limit);
      if (listret < 0) {
	fprintf(stderr, "Could not add new realm to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_realm_list(realms);
  ctxt->node = oldnode;

  return ret;
}

static int get_realms(struct deltacloud_api *api, struct realm **realms)
{
  struct link *thislink;
  char *data;
  int ret = -1;

  thislink = find_by_rel_in_link_list(&api->links, "realms");
  if (thislink == NULL) {
    fprintf(stderr, "Could not find the link for 'realms'\n");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not fetch XML for realms from url %s\n",
	    thislink->href);
    return -1;
  }

  *realms = NULL;
  if (parse_xml(data, "realms", (void **)realms, parse_realm_xml) < 0) {
    fprintf(stderr, "Could not parse realms XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int get_realm_by_id(struct deltacloud_api *api, const char *id,
			   struct realm *realm)
{
  char *url, *data;
  struct realm *tmprealm = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;

  if (asprintf(&url, "%s/realms/%s", api->url, id) < 0) {
    fprintf(stderr, "Failed to allocate memory for url\n");
    return -1;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML data from url %s\n", url);
    goto cleanup;
  }

  xml = xmlReadDoc(BAD_CAST data, "realm.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    fprintf(stderr, "Failed to parse XML\n");
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    fprintf(stderr, "Failed to get the root element\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  if (parse_realm_xml(root, ctxt, (void **)&tmprealm) < 0) {
    fprintf(stderr, "Could not parse 'flavor' XML\n");
    goto cleanup;
  }

  if (copy_realm(realm, tmprealm) < 0) {
    fprintf(stderr, "Could not copy realm structure\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  free_realm_list(&tmprealm);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  MY_FREE(data);
  MY_FREE(url);

  return ret;
}

static int parse_flavor_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			    void **data)
{
  struct flavor **flavors = (struct flavor **)data;
  xmlNodePtr flavor_cur, oldnode;
  char *href = NULL, *id = NULL, *memory = NULL, *storage = NULL;
  char *architecture = NULL;
  int listret;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "flavor")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");

      ctxt->node = cur;
      flavor_cur = cur->children;
      while (flavor_cur != NULL) {
	if (flavor_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)flavor_cur->name, "id"))
	    id = getXPathString("string(./id)", ctxt);
	  else if (STREQ((const char *)flavor_cur->name, "memory"))
	    memory = getXPathString("string(./memory)", ctxt);
	  else if (STREQ((const char *)flavor_cur->name, "storage"))
	    storage = getXPathString("string(./storage)", ctxt);
	  else if (STREQ((const char *)flavor_cur->name, "architecture"))
	    architecture = getXPathString("string(./architecture)", ctxt);
	}
	flavor_cur = flavor_cur->next;
      }
      listret = add_to_flavor_list(flavors, href, id, memory, storage,
				   architecture);
      MY_FREE(href);
      MY_FREE(id);
      MY_FREE(memory);
      MY_FREE(storage);
      MY_FREE(architecture);
      if (listret < 0) {
	fprintf(stderr, "Could not add new flavor to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    free_flavor_list(flavors);

  return ret;
}

static int get_flavors(struct deltacloud_api *api, struct flavor **flavors)
{
  struct link *thislink;
  char *data;
  int ret = -1;

  thislink = find_by_rel_in_link_list(&api->links, "flavors");
  if (thislink == NULL) {
    fprintf(stderr, "Could not find the link for 'flavors'\n");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML data from url %s\n", thislink->href);
    return -1;
  }

  *flavors = NULL;
  if (parse_xml(data, "flavors", (void **)flavors, parse_flavor_xml) < 0) {
    fprintf(stderr, "Could not parse XML data for flavors\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int get_flavor_by_id(struct deltacloud_api *api, const char *id,
			    struct flavor *flavor)
{
  struct link *thislink;
  char *data, *fullurl;
  int ret = -1;
  struct flavor *tmpflavor = NULL;

  thislink = find_by_rel_in_link_list(&api->links, "flavors");
  if (thislink == NULL) {
    fprintf(stderr, "Could not find the link for 'flavors'\n");
    return -1;
  }

  if (asprintf(&fullurl, "%s?id=%s", thislink->href, id) < 0) {
    fprintf(stderr, "Could not allocate memory for fullurl\n");
    return -1;
  }

  data = get_url(fullurl, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML data from url %s\n", fullurl);
    goto cleanup;
  }

  if (parse_xml(data, "flavors", (void **)&tmpflavor, parse_flavor_xml) < 0) {
    fprintf(stderr, "Could not parse XML data for flavors\n");
    goto cleanup;
  }

  if (copy_flavor(flavor, tmpflavor) < 0) {
    fprintf(stderr, "Could not copy flavor structure\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  free_flavor_list(&tmpflavor);
  MY_FREE(data);
  MY_FREE(fullurl);

  return ret;
}

static int get_flavor_by_uri(struct deltacloud_api *api, const char *url,
			     struct flavor *flavor)
{
  char *data;
  struct flavor *tmpflavor = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML data from url %s\n", url);
    return -1;
  }

  xml = xmlReadDoc(BAD_CAST data, "flavor.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    fprintf(stderr, "Failed to parse XML\n");
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    fprintf(stderr, "Failed to get the root element\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  if (parse_flavor_xml(root, ctxt, (void **)&tmpflavor) < 0) {
    fprintf(stderr, "Could not parse 'flavor' XML\n");
    goto cleanup;
  }

  if (copy_flavor(flavor, tmpflavor) < 0) {
    fprintf(stderr, "Could not copy flavor structure\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  free_flavor_list(&tmpflavor);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  MY_FREE(data);

  return ret;
}

static int parse_image_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			   void **data)
{
  struct image **images = (struct image **)data;
  xmlNodePtr oldnode, image_cur;
  char *href = NULL, *id = NULL, *description = NULL, *architecture = NULL;
  char *owner_id = NULL, *name = NULL;
  int listret;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "image")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");

      ctxt->node = cur;
      image_cur = cur->children;
      while (image_cur != NULL) {
	if (image_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)image_cur->name, "id"))
	    id = getXPathString("string(./id)", ctxt);
	  else if (STREQ((const char *)image_cur->name, "description"))
	    description = getXPathString("string(./description)", ctxt);
	  else if (STREQ((const char *)image_cur->name, "architecture"))
	    architecture = getXPathString("string(./architecture)", ctxt);
	  else if (STREQ((const char *)image_cur->name, "owner_id"))
	    owner_id = getXPathString("string(./owner_id)", ctxt);
	  else if (STREQ((const char *)image_cur->name, "name"))
	    name = getXPathString("string(./name)", ctxt);
	}
	image_cur = image_cur->next;
      }
      listret = add_to_image_list(images, href, id, description, architecture,
				  owner_id, name);
      MY_FREE(href);
      MY_FREE(id);
      MY_FREE(description);
      MY_FREE(architecture);
      MY_FREE(owner_id);
      MY_FREE(name);
      if (listret < 0) {
	fprintf(stderr, "Could not add new image to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    free_image_list(images);

  return ret;
}

static int get_images(struct deltacloud_api *api, struct image **images)
{
  struct link *thislink;
  char *data;
  int ret = -1;

  thislink = find_by_rel_in_link_list(&api->links, "images");
  if (thislink == NULL) {
    fprintf(stderr, "Could not find the link for 'images'\n");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get the XML from images url %s\n",
	    thislink->href);
    return -1;
  }

  *images = NULL;
  if (parse_xml(data, "images", (void **)images, parse_image_xml) < 0) {
    fprintf(stderr, "Could not parse images XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int get_image_by_id(struct deltacloud_api *api, const char *id,
			   struct image *image)
{
  char *url, *data;
  struct image *tmpimage = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;

  if (asprintf(&url, "%s/images/%s", api->url, id) < 0) {
    fprintf(stderr, "Failed to allocate memory for url\n");
    return -1;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML data from url %s\n", url);
    goto cleanup;
  }

  xml = xmlReadDoc(BAD_CAST data, "image.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    fprintf(stderr, "Failed to parse XML\n");
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    fprintf(stderr, "Failed to get the root element\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  if (parse_image_xml(root, ctxt, (void **)&tmpimage) < 0) {
    fprintf(stderr, "Could not parse 'image' XML\n");
    goto cleanup;
  }

  if (copy_image(image, tmpimage) < 0) {
    fprintf(stderr, "Could not copy image structure\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  free_image_list(&tmpimage);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  MY_FREE(data);
  MY_FREE(url);

  return ret;
}

static int parse_instance_state_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void **data)
{
  struct instance_state **instance_states = (struct instance_state **)data;
  xmlNodePtr state_cur;
  char *name = NULL, *action = NULL, *to = NULL;
  struct transition *transitions = NULL;
  int ret = -1;
  int listret;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "state")) {
      name = (char *)xmlGetProp(cur, BAD_CAST "name");

      state_cur = cur->children;
      while (state_cur != NULL) {
	if (state_cur->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)state_cur->name, "transition")) {
	  action = (char *)xmlGetProp(state_cur, BAD_CAST "action");
	  to = (char *)xmlGetProp(state_cur, BAD_CAST "to");
	  add_to_transition_list(&transitions, action, to);
	  MY_FREE(action);
	  MY_FREE(to);
	}
	state_cur = state_cur->next;
      }
      listret = add_to_instance_state_list(instance_states, name, transitions);
      MY_FREE(name);
      free_transition_list(&transitions);
      if (listret < 0) {
	fprintf(stderr, "Could not add new instance_state to list\n");
	goto cleanup;
      }
      transitions = NULL;
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_instance_state_list(instance_states);

  return ret;
}

static int get_instance_states(struct deltacloud_api *api,
			       struct instance_state **instance_states)
{
  struct link *thislink;
  char *data;
  int ret = -1;

  thislink = find_by_rel_in_link_list(&api->links, "instance_states");
  if (thislink == NULL) {
    fprintf(stderr, "Could not find the link for 'instance-states'\n");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML for instance_states from url %s\n",
	    thislink->href);
    return -1;
  }

  *instance_states = NULL;
  if (parse_xml(data, "states", (void **)instance_states, parse_instance_state_xml) < 0) {
    fprintf(stderr, "Could not parse instance_states XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int get_instance_state(struct deltacloud_api *api, const char *name,
			      struct instance_state *instance_state)
{
  struct instance_state *statelist = NULL;
  struct instance_state *found;
  int ret = -1;

  if (get_instance_states(api, &statelist) < 0) {
    fprintf(stderr, "Failed to get instance_states\n");
    return -1;
  }
  found = find_by_name_in_instance_state_list(&statelist, name);
  if (found == NULL) {
    fprintf(stderr, "Could not find %s in instance state list\n", name);
    goto cleanup;
  }
  if (copy_instance_state(instance_state, found) < 0) {
    fprintf(stderr, "Could not copy instance_state structure\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  free_instance_state_list(&statelist);
  return ret;
}

static int parse_storage_volume_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void **data)
{
  struct storage_volume **storage_volumes = (struct storage_volume **)data;
  xmlNodePtr oldnode, storage_cur;
  int ret = -1;
  char *href = NULL, *id = NULL, *created = NULL, *state = NULL;
  char *capacity = NULL, *device = NULL, *instance_href = NULL;
  int listret;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "storage-volume")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");

      ctxt->node = cur;
      storage_cur = cur->children;
      while (storage_cur != NULL) {
	if (storage_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)storage_cur->name, "id"))
	    id = getXPathString("string(./id)", ctxt);
	  else if (STREQ((const char *)storage_cur->name, "created"))
	    created = getXPathString("string(./created)", ctxt);
	  else if (STREQ((const char *)storage_cur->name, "state"))
	    state = getXPathString("string(./state)", ctxt);
	  else if (STREQ((const char *)storage_cur->name, "capacity"))
	    capacity = getXPathString("string(./capacity)", ctxt);
	  else if (STREQ((const char *)storage_cur->name, "device"))
	    device = getXPathString("string(./device)", ctxt);
	  else if (STREQ((const char *)storage_cur->name, "instance"))
	    instance_href = (char *)xmlGetProp(storage_cur, BAD_CAST "href");
	}
	storage_cur = storage_cur->next;
      }
      listret = add_to_storage_volume_list(storage_volumes, href, id, created,
					   state, capacity, device,
					   instance_href);
      MY_FREE(id);
      MY_FREE(created);
      MY_FREE(state);
      MY_FREE(capacity);
      MY_FREE(device);
      MY_FREE(instance_href);
      MY_FREE(href);
      if (listret < 0) {
	fprintf(stderr, "Could not add new storage_volume to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    free_storage_volume_list(storage_volumes);

  return ret;
}

static int get_storage_volumes(struct deltacloud_api *api,
			       struct storage_volume **storage_volumes)
{
  struct link *thislink;
  char *data;
  int ret = -1;

  thislink = find_by_rel_in_link_list(&api->links, "storage_volumes");
  if (thislink == NULL) {
    fprintf(stderr, "Could not find the link for 'storage_volumes'\n");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML for storage_volumes from url %s\n",
	    thislink->href);
    return -1;
  }

  *storage_volumes = NULL;
  if (parse_xml(data, "storage-volumes", (void **)storage_volumes, parse_storage_volume_xml) < 0) {
    fprintf(stderr, "Could not parse storage_volumes XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int get_storage_volume_by_id(struct deltacloud_api *api, const char *id,
				    struct storage_volume *storage_volume)
{
  char *url, *data;
  struct storage_volume *tmpstorage_volume = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;

  if (asprintf(&url, "%s/storage_volumes/%s", api->url, id) < 0) {
    fprintf(stderr, "Failed to allocate memory for url\n");
    return -1;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML data from url %s\n", url);
    goto cleanup;
  }

  xml = xmlReadDoc(BAD_CAST data, "storage_volume.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    fprintf(stderr, "Failed to parse XML\n");
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    fprintf(stderr, "Failed to get the root element\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  if (parse_storage_volume_xml(root, ctxt, (void **)&tmpstorage_volume) < 0) {
    fprintf(stderr, "Could not parse 'instance' XML\n");
    goto cleanup;
  }

  if (copy_storage_volume(storage_volume, tmpstorage_volume) < 0) {
    fprintf(stderr, "Could not copy storage_volume structure\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  free_storage_volume_list(&tmpstorage_volume);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  MY_FREE(data);
  MY_FREE(url);

  return ret;
}

static int parse_storage_snapshot_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data)
{
  struct storage_snapshot **storage_snapshots = (struct storage_snapshot **)data;
  int ret = -1;
  xmlNodePtr oldnode, snap_cur;
  char *href = NULL, *id = NULL, *created = NULL, *state = NULL;
  char *storage_volume_href = NULL;
  int listret;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "storage-snapshot")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");

      ctxt->node = cur;
      snap_cur = cur->children;
      while (snap_cur != NULL) {
	if (snap_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)snap_cur->name, "id"))
	    id = getXPathString("string(./id)", ctxt);
	  else if (STREQ((const char *)snap_cur->name, "created"))
	    created = getXPathString("string(./created)", ctxt);
	  else if (STREQ((const char *)snap_cur->name, "state"))
	    state = getXPathString("string(./state)", ctxt);
	  else if (STREQ((const char *)snap_cur->name, "storage-volume"))
	    storage_volume_href = (char *)xmlGetProp(snap_cur, BAD_CAST "href");
	}
	snap_cur = snap_cur->next;
      }
      listret = add_to_storage_snapshot_list(storage_snapshots, href, id,
					     created, state,
					     storage_volume_href);
      MY_FREE(id);
      MY_FREE(created);
      MY_FREE(state);
      MY_FREE(storage_volume_href);
      MY_FREE(href);
      if (listret < 0) {
	fprintf(stderr, "Could not add new storage_snapshot to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    free_storage_snapshot_list(storage_snapshots);

  return ret;
}

static int get_storage_snapshots(struct deltacloud_api *api,
				 struct storage_snapshot **storage_snapshots)
{
  struct link *thislink;
  char *data;
  int ret = -1;

  thislink = find_by_rel_in_link_list(&api->links, "storage_snapshots");
  if (thislink == NULL) {
    fprintf(stderr, "Could not find the link for 'storage_snapshots'\n");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML for storage_snapshots from url %s\n",
	    thislink->href);
    return -1;
  }

  *storage_snapshots = NULL;
  if (parse_xml(data, "storage-snapshots", (void **)storage_snapshots, parse_storage_snapshot_xml) < 0) {
    fprintf(stderr, "Could not parse storage_snapshots XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int get_storage_snapshot_by_id(struct deltacloud_api *api,
				      const char *id,
				      struct storage_snapshot *storage_snapshot)
{
  char *url, *data;
  struct storage_snapshot *tmpstorage_snapshot = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;

  if (asprintf(&url, "%s/storage_snapshots/%s", api->url, id) < 0) {
    fprintf(stderr, "Failed to allocate memory for url\n");
    return -1;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML data from url %s\n", url);
    goto cleanup;
  }

  xml = xmlReadDoc(BAD_CAST data, "storage_snapshot.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    fprintf(stderr, "Failed to parse XML\n");
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    fprintf(stderr, "Failed to get the root element\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  if (parse_storage_snapshot_xml(root, ctxt, (void **)&tmpstorage_snapshot) < 0) {
    fprintf(stderr, "Could not parse 'storage_snapshot' XML\n");
    goto cleanup;
  }

  if (copy_storage_snapshot(storage_snapshot, tmpstorage_snapshot) < 0) {
    fprintf(stderr, "Could not copy storage_snapshot structure\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  free_storage_snapshot_list(&tmpstorage_snapshot);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  MY_FREE(data);
  MY_FREE(url);

  return ret;
}

static struct instance *create_instance(struct deltacloud_api *api,
					const char *image_id, const char *name,
					const char *realm_id,
					const char *flavor_id)
{
  struct link *thislink;
  char *data, *params;
  size_t param_size;
  FILE *paramfp;
  struct instance *newinstance = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;

  if (image_id == NULL) {
    fprintf(stderr, "Image ID cannot be NULL\n");
    return NULL;
  }

  thislink = find_by_rel_in_link_list(&api->links, "instances");
  if (thislink == NULL) {
    fprintf(stderr, "Could not find the link for 'instances'\n");
    return NULL;
  }

  paramfp = open_memstream(&params, &param_size);
  if (paramfp == NULL) {
    fprintf(stderr, "Could not allocate memory for parameters\n");
    return NULL;
  }

  fprintf(paramfp, "image_id=%s", image_id);
  if (name != NULL)
    fprintf(paramfp, "&name=%s", name);
  if (realm_id != NULL)
    fprintf(paramfp, "&realm_id=%s", realm_id);
  if (flavor_id != NULL)
    fprintf(paramfp, "&flavor_id=%s", flavor_id);
  fclose(paramfp);

  data = post_url(thislink->href, api->user, api->password, params, param_size);
  MY_FREE(params);
  if (data == NULL) {
    fprintf(stderr, "Failed to post instance data\n");
    return NULL;
  }

  xml = xmlReadDoc(BAD_CAST data, "instance.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    fprintf(stderr, "Failed to parse XML\n");
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    fprintf(stderr, "Failed to get the root element\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  if (parse_instance_xml(root, ctxt, (void **)&newinstance) < 0) {
    fprintf(stderr, "Could not parse 'instance' XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);

  MY_FREE(data);

  return newinstance;
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct instance *instances;
  struct image *images;
  struct flavor *flavors;
  struct realm *realms;
  struct instance_state *instance_states;
  struct storage_volume *storage_volumes;
  struct storage_snapshot *storage_snapshots;
  struct flavor flavor;
  struct instance_state instance_state;
  struct realm realm;
  struct image image;
  struct instance instance;
  struct storage_volume storage_volume;
  struct storage_snapshot storage_snapshot;
  struct instance *newinstance;
  char *fullurl;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  api.url = argv[1];
  api.user = argv[2];
  api.password = argv[3];

  fprintf(stderr, "--------------LINKS--------------------------\n");
  if (get_links(&api) < 0) {
    fprintf(stderr, "Failed to find links for the API\n");
    return 2;
  }
  print_link_list(&api.links, NULL);

  fprintf(stderr, "--------------IMAGES-------------------------\n");
  if (get_images(&api, &images) < 0) {
    fprintf(stderr, "Failed to get_images\n");
    goto cleanup;
  }
  print_image_list(&images, NULL);
  free_image_list(&images);

  if (get_image_by_id(&api, "img1", &image) < 0) {
    fprintf(stderr, "Failed to get image by id\n");
    goto cleanup;
  }
  print_image(&image, NULL);
  free_image(&image);

  fprintf(stderr, "--------------FLAVORS------------------------\n");
  if (get_flavors(&api, &flavors) < 0) {
    fprintf(stderr, "Failed to get_flavors\n");
    goto cleanup;
  }
  print_flavor_list(&flavors, NULL);
  free_flavor_list(&flavors);

  if (get_flavor_by_id(&api, "m1-small", &flavor) < 0) {
    fprintf(stderr, "Failed to get 'm1-small' flavor\n");
    goto cleanup;
  }
  print_flavor(&flavor, NULL);
  free_flavor(&flavor);

  if (asprintf(&fullurl, "%s/flavors/c1-medium", api.url) < 0) {
    fprintf(stderr, "Failed to allocate fullurl\n");
    goto cleanup;
  }
  if (get_flavor_by_uri(&api, fullurl, &flavor) < 0) {
    fprintf(stderr, "Failed to get 'c1-medium' flavor\n");
    MY_FREE(fullurl);
    goto cleanup;
  }
  print_flavor(&flavor, NULL);
  free_flavor(&flavor);
  MY_FREE(fullurl);

  fprintf(stderr, "--------------REALMS-------------------------\n");
  if (get_realms(&api, &realms) < 0) {
    fprintf(stderr, "Failed to get_realms\n");
    goto cleanup;
  }
  print_realm_list(&realms, NULL);
  free_realm_list(&realms);

  if (get_realm_by_id(&api, "us", &realm) < 0) {
    fprintf(stderr, "Failed to get realm by id\n");
    goto cleanup;
  }
  print_realm(&realm, NULL);
  free_realm(&realm);

  fprintf(stderr, "--------------INSTANCE STATES----------------\n");
  if (get_instance_states(&api, &instance_states) < 0) {
    fprintf(stderr, "Failed to get_instance_states\n");
    goto cleanup;
  }
  print_instance_state_list(&instance_states, NULL);
  free_instance_state_list(&instance_states);

  if (get_instance_state(&api, "start", &instance_state) < 0) {
    fprintf(stderr, "Failed to get instance_state\n");
    goto cleanup;
  }
  print_instance_state(&instance_state, NULL);
  free_instance_state(&instance_state);

  fprintf(stderr, "--------------INSTANCES---------------------\n");
  if (get_instances(&api, &instances) < 0) {
    fprintf(stderr, "Failed to get_instances\n");
    goto cleanup;
  }
  print_instance_list(&instances, NULL);
  free_instance_list(&instances);

  if (get_instance_by_id(&api, "inst18", &instance) < 0) {
    fprintf(stderr, "Failed to get instance by id\n");
    goto cleanup;
  }
  print_instance(&instance, NULL);
  free_instance(&instance);

  fprintf(stderr, "--------------STORAGE VOLUMES---------------\n");
  if (get_storage_volumes(&api, &storage_volumes) < 0) {
    fprintf(stderr, "Failed to get_storage_volumes\n");
    goto cleanup;
  }
  print_storage_volume_list(&storage_volumes, NULL);
  free_storage_volume_list(&storage_volumes);

  if (get_storage_volume_by_id(&api, "vol3", &storage_volume) < 0) {
    fprintf(stderr, "Failed to get storage volume by ID\n");
    goto cleanup;
  }
  print_storage_volume(&storage_volume, NULL);
  free_storage_volume(&storage_volume);

  fprintf(stderr, "--------------STORAGE SNAPSHOTS-------------\n");
  if (get_storage_snapshots(&api, &storage_snapshots) < 0) {
    fprintf(stderr, "Failed to get_storage_snapshots\n");
    goto cleanup;
  }
  print_storage_snapshot_list(&storage_snapshots, NULL);
  free_storage_snapshot_list(&storage_snapshots);

  if (get_storage_snapshot_by_id(&api, "snap2", &storage_snapshot) < 0) {
    fprintf(stderr, "Failed to get storage_snapshot by ID\n");
    goto cleanup;
  }
  print_storage_snapshot(&storage_snapshot, NULL);
  free_storage_snapshot(&storage_snapshot);

  fprintf(stderr, "--------------CREATE INSTANCE---------------\n");
  newinstance = create_instance(&api, "img3", NULL, NULL, NULL);
  if (newinstance == NULL) {
    fprintf(stderr, "Failed to create_instance\n");
    goto cleanup;
  }
  print_instance(newinstance, NULL);
  free_instance(newinstance);
  MY_FREE(newinstance);

  ret = 0;

 cleanup:
  free_link_list(&api.links);

  return ret;
}
