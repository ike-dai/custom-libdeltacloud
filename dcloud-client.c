#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include "common.h"
#include "geturl.h"
#include "links.h"
#include "instances.h"
#include "realms.h"
#include "flavors.h"
#include "images.h"
#include "instance_states.h"
#include "storage_volumes.h"
#include "storage_snapshots.h"

struct deltacloud_api {
  char *url;
  char *user;
  char *password;

  struct link *links;
};

static int parse_api_xml(const char *xml_string, struct link **links)
{
  xmlDocPtr xml;
  xmlNodePtr node, cur;
  char *href = NULL, *rel = NULL;
  int ret = -1;
  int listret;

  xml = xmlReadDoc(BAD_CAST xml_string, "api.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    fprintf(stderr, "Failed to parse XML\n");
    return -1;
  }

  node = xmlDocGetRootElement(xml);
  if (node == NULL) {
    fprintf(stderr, "Failed to get the root element\n");
    goto cleanup;
  }

  if (STRNEQ((const char *)node->name, "api")) {
    fprintf(stderr, "Root element was not 'api'\n");
    goto cleanup;
  }

  cur = node->children;
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
  xmlFreeDoc(xml);

  return ret;
}

static int get_links(char *url, char *user, char *password, struct link **links)
{
  char *data;
  int ret = -1;

  data = get_url(url, user, password);
  if (data == NULL) {
    fprintf(stderr, "Failed to get data from url %s\n",url);
    return -1;
  }
  if (parse_api_xml(data, links) < 0) {
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
  int failed = 0;
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

static int parse_instances_xml(char *xml_string, struct instance **instances)
{
  xmlDocPtr xml;
  xmlNodePtr root, cur, instance_cur;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;
  char *id = NULL, *name = NULL, *owner_id = NULL, *image_href = NULL;
  char *flavor_href = NULL, *realm_href = NULL;
  struct action *actions = NULL;
  struct address *public_addresses = NULL;
  struct address *private_addresses = NULL;
  int listret;

  xml = xmlReadDoc(BAD_CAST xml_string, "instances.xml", NULL,
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

  if (STRNEQ((const char *)root->name, "instances")) {
    fprintf(stderr, "Root element was not 'instances'\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  cur = root->children;
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
				     flavor_href, realm_href, actions,
				     public_addresses, private_addresses);
      MY_FREE(id);
      MY_FREE(name);
      MY_FREE(owner_id);
      MY_FREE(image_href);
      MY_FREE(flavor_href);
      MY_FREE(realm_href);
      if (listret < 0) {
	free_address_list(&public_addresses);
	free_address_list(&private_addresses);
	free_action_list(&actions);
	fprintf(stderr, "Failed to add new instance to list\n");
	goto cleanup;
      }
      actions = NULL;
      public_addresses = NULL;
      private_addresses = NULL;
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_instance_list(instances);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);
  return ret;
}

static int get_instances(char *user, char *password, struct link **links,
			 struct instance **instances)
{
  char *url, *data;
  int ret = -1;

  url = find_href_by_rel_in_link_list(links, "instances");
  if (url == NULL) {
    fprintf(stderr, "Could not find the link for 'instances'\n");
    return -1;
  }

  data = get_url(url, user, password);
  if (data == NULL) {
    fprintf(stderr, "Could not get instance data\n");
    return -1;
  }
  if (parse_instances_xml(data, instances) < 0) {
    fprintf(stderr, "Could not parse instances XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int parse_realms_xml(char *xml_string, struct realm **realms)
{
  xmlDocPtr xml;
  xmlNodePtr root, cur, realm_cur;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;
  char *href = NULL, *id = NULL, *name = NULL, *state = NULL, *limit = NULL;
  int listret;

  xml = xmlReadDoc(BAD_CAST xml_string, "realms.xml", NULL,
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

  if (STRNEQ((const char *)root->name, "realms")) {
    fprintf(stderr, "Root element was not 'realms'\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  cur = root->children;
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
    free_realms_list(realms);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);
  return ret;
}

static int get_realms(char *user, char *password, struct link **links,
		      struct realm **realms)
{
  char *url, *data;
  int ret = -1;

  url = find_href_by_rel_in_link_list(links, "realms");
  if (url == NULL) {
    fprintf(stderr, "Could not find the link for 'realms'\n");
    return -1;
  }

  data = get_url(url, user, password);
  if (data == NULL) {
    fprintf(stderr, "Could not fetch realms XML\n");
    return -1;
  }

  if (parse_realms_xml(data, realms) < 0) {
    fprintf(stderr, "Could not parse realms XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int parse_flavors_xml(char *xml_string, struct flavor **flavors)
{
  xmlDocPtr xml;
  xmlNodePtr root, cur, flavor_cur;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;
  char *href = NULL, *id = NULL, *memory = NULL, *storage = NULL;
  char *architecture = NULL;
  int listret;

  xml = xmlReadDoc(BAD_CAST xml_string, "flavors.xml", NULL,
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

  if (STRNEQ((const char *)root->name, "flavors")) {
    fprintf(stderr, "Root element was not 'flavors'\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  cur = root->children;
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
      listret = add_to_flavor_list(flavors, href, id, memory, storage, architecture);
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
  if (ret < 0)
    free_flavors_list(flavors);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);
  return ret;
}

static int get_flavors(char *user, char *password, struct link **links,
		       struct flavor **flavors)
{
  char *url, *data;
  int ret = -1;

  url = find_href_by_rel_in_link_list(links, "flavors");
  if (url == NULL) {
    fprintf(stderr, "Could not find the link for 'flavors'\n");
    return -1;
  }

  data = get_url(url, user, password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML data for flavors\n");
    return -1;
  }

  if (parse_flavors_xml(data, flavors) < 0) {
    fprintf(stderr, "Could not parse XML data for flavors\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int parse_images_xml(char *xml_string, struct image **images)
{
  xmlDocPtr xml;
  xmlNodePtr root, cur, image_cur;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;
  char *href = NULL, *id = NULL, *description = NULL, *architecture = NULL;
  char *owner_id = NULL;
  int listret;

  xml = xmlReadDoc(BAD_CAST xml_string, "images.xml", NULL,
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

  if (STRNEQ((const char *)root->name, "images")) {
    fprintf(stderr, "Root element was not 'images'\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  cur = root->children;
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
	}
	image_cur = image_cur->next;
      }
      listret = add_to_image_list(images, href, id, description, architecture,
				  owner_id);
      MY_FREE(href);
      MY_FREE(id);
      MY_FREE(description);
      MY_FREE(architecture);
      MY_FREE(owner_id);
      if (listret < 0) {
	fprintf(stderr, "Could not add new image to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_images_list(images);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);
  return ret;
}

static int get_images(char *user, char *password, struct link **links,
		      struct image **images)
{
  char *url, *data;
  int ret = -1;

  url = find_href_by_rel_in_link_list(links, "images");
  if (url == NULL) {
    fprintf(stderr, "Could not find the link for 'images'\n");
    return -1;
  }

  data = get_url(url, user, password);
  if (data == NULL) {
    fprintf(stderr, "Could not get the XML for images\n");
    return -1;
  }

  if (parse_images_xml(data, images) < 0) {
    fprintf(stderr, "Could not parse images XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int parse_instance_states_xml(char *xml_string,
				     struct instance_state **instance_states)
{
  xmlDocPtr xml;
  xmlNodePtr root, cur, state_cur;
  int ret = -1;
  char *name = NULL, *action = NULL, *to = NULL;
  struct transition *transitions = NULL;
  int listret;

  xml = xmlReadDoc(BAD_CAST xml_string, "instance_states.xml", NULL,
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

  if (STRNEQ((const char *)root->name, "states")) {
    fprintf(stderr, "Root element was not 'states'\n");
    goto cleanup;
  }

  cur = root->children;
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
	  add_to_transitions_list(&transitions, action, to);
	  MY_FREE(action);
	  MY_FREE(to);
	}
	state_cur = state_cur->next;
      }
      listret = add_to_instance_states_list(instance_states, name, transitions);
      MY_FREE(name);
      if (listret < 0) {
	fprintf(stderr, "Could not add new instance_state to list\n");
	free_transitions_list(&transitions);
	goto cleanup;
      }
      transitions = NULL;
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    free_instance_states_list(instance_states);
  xmlFreeDoc(xml);
  return ret;
}

static int get_instance_states(char *user, char *password, struct link **links,
			       struct instance_state **instance_states)
{
  char *url, *data;
  int ret = -1;

  url = find_href_by_rel_in_link_list(links, "instance_states");
  if (url == NULL) {
    fprintf(stderr, "Could not find the link for 'instance-states'\n");
    return -1;
  }

  data = get_url(url, user, password);
  if (data == NULL) {
    fprintf(stderr, "Could not get instance_states XML\n");
    return -1;
  }

  if (parse_instance_states_xml(data, instance_states) < 0) {
    fprintf(stderr, "Could not parse instance_states XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int parse_storage_volumes_xml(char *xml_string,
				     struct storage_volume **storage_volumes)
{
  xmlDocPtr xml;
  xmlNodePtr root, cur, storage_cur;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;
  char *href = NULL, *id = NULL, *created = NULL, *state = NULL;
  char *capacity = NULL, *device = NULL, *instance_href = NULL;
  int listret;

  xml = xmlReadDoc(BAD_CAST xml_string, "storage_volumes.xml", NULL,
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

  if (STRNEQ((const char *)root->name, "storage-volumes")) {
    fprintf(stderr, "Root element was not 'storage-volumes'\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  cur = root->children;
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
  if (ret < 0)
    free_storage_volume_list(storage_volumes);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);
  return ret;
}

static int get_storage_volumes(char *user, char *password, struct link **links,
			       struct storage_volume **storage_volumes)
{
  char *url, *data;
  int ret = -1;

  url = find_href_by_rel_in_link_list(links, "storage_volumes");
  if (url == NULL) {
    fprintf(stderr, "Could not find the link for 'storage_volumes'\n");
    return -1;
  }

  data = get_url(url, user, password);
  if (data == NULL) {
    fprintf(stderr, "Could not get data for storage_volumes\n");
    return -1;
  }

  if (parse_storage_volumes_xml(data, storage_volumes) < 0) {
    fprintf(stderr, "Could not parse storage_volumes XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int parse_storage_snapshots_xml(char *xml_string,
				       struct storage_snapshot **storage_snapshots)
{
  xmlDocPtr xml;
  xmlNodePtr root, cur, snap_cur;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;
  char *href = NULL, *id = NULL, *created = NULL, *state = NULL;
  char *storage_volume_href = NULL;
  int listret;

  xml = xmlReadDoc(BAD_CAST xml_string, "storage_snapshots.xml", NULL,
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

  if (STRNEQ((const char *)root->name, "storage-snapshots")) {
    fprintf(stderr, "Root element was not 'storage-snapshots'\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    fprintf(stderr, "Could not initialize XML parsing\n");
    goto cleanup;
  }

  cur = root->children;
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
  if (ret < 0)
    free_storage_snapshot_list(storage_snapshots);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);
  return ret;
}

static int get_storage_snapshots(char *user, char *password,
				 struct link **links,
				 struct storage_snapshot **storage_snapshots)
{
  char *url, *data;
  int ret = -1;

  url = find_href_by_rel_in_link_list(links, "storage_snapshots");
  if (url == NULL) {
    fprintf(stderr, "Could not find the link for 'storage_snapshots'\n");
    return -1;
  }

  data = get_url(url, user, password);
  if (data == NULL) {
    fprintf(stderr, "Could not get XML for storage_snapshots\n");
    return -1;
  }

  if (parse_storage_snapshots_xml(data, storage_snapshots) < 0) {
    fprintf(stderr, "Could not parse storage_snapshots XML\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  MY_FREE(data);

  return ret;
}

static int create_instance(char *user, char *password, struct link **links)
{
  char *url;
  char *data;

  url = find_href_by_rel_in_link_list(links, "instances");
  if (url == NULL) {
    fprintf(stderr, "Could not find the link for 'instances'\n");
    return -1;
  }

  data = post_url(url, user, password, "image_id=img3", strlen("image_id=img3"));
  fprintf(stderr, "After create_instance: %s\n",data);
  MY_FREE(data);

  return 0;
}

int main(int argc, char *argv[])
{
  char *url, *user, *password;
  struct link *apilinks = NULL;
  struct deltacloud_api api;
  struct instance *instances = NULL;
  struct image *images = NULL;
  struct flavor *flavors = NULL;
  struct realm *realms = NULL;
  struct instance_state *instance_states = NULL;
  struct storage_volume *storage_volumes = NULL;
  struct storage_snapshot *storage_snapshots = NULL;
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  url = argv[1];
  user = argv[2];
  password = argv[3];

  if (get_links(url, user, password, &apilinks) < 0) {
    fprintf(stderr, "Failed to find links for the API\n");
    return 2;
  }
  fprintf(stderr, "--------------LINKS--------------------------\n");
  print_link_list(&apilinks, NULL);

  if (get_images(user, password, &apilinks, &images) < 0) {
    fprintf(stderr, "Failed to get_images\n");
    goto cleanup;
  }
  fprintf(stderr, "--------------IMAGES-------------------------\n");
  print_images_list(&images, NULL);
  free_images_list(&images);

  if (get_flavors(user, password, &apilinks, &flavors) < 0) {
    fprintf(stderr, "Failed to get_flavors\n");
    goto cleanup;
  }
  fprintf(stderr, "--------------FLAVORS------------------------\n");
  print_flavors_list(&flavors, NULL);
  free_flavors_list(&flavors);

  if (get_realms(user, password, &apilinks, &realms) < 0) {
    fprintf(stderr, "Failed to get_realms\n");
    goto cleanup;
  }
  fprintf(stderr, "--------------REALMS-------------------------\n");
  print_realms_list(&realms, NULL);
  free_realms_list(&realms);

  if (get_instance_states(user, password, &apilinks, &instance_states) < 0) {
    fprintf(stderr, "Failed to get_instance_states\n");
    goto cleanup;
  }
  fprintf(stderr, "--------------INSTANCE STATES----------------\n");
  print_instance_states_list(&instance_states, NULL);
  free_instance_states_list(&instance_states);

  if (get_instances(user, password, &apilinks, &instances) < 0) {
    fprintf(stderr, "Failed to get_instances\n");
    goto cleanup;
  }
  fprintf(stderr, "--------------INSTANCES---------------------\n");
  print_instance_list(&instances, NULL);
  free_instance_list(&instances);

  if (get_storage_volumes(user, password, &apilinks, &storage_volumes) < 0) {
    fprintf(stderr, "Failed to get_storage_volumes\n");
    goto cleanup;
  }
  fprintf(stderr, "--------------STORAGE VOLUMES---------------\n");
  print_storage_volume_list(&storage_volumes, NULL);
  free_storage_volume_list(&storage_volumes);

  if (get_storage_snapshots(user, password, &apilinks, &storage_snapshots) < 0) {
    fprintf(stderr, "Failed to get_storage_snapshots\n");
    goto cleanup;
  }
  fprintf(stderr, "--------------STORAGE SNAPSHOTS-------------\n");
  print_storage_snapshot_list(&storage_snapshots, NULL);
  free_storage_snapshot_list(&storage_snapshots);

  fprintf(stderr, "--------------CREATE INSTANCE---------------\n");
  if (create_instance(user, password, &apilinks) < 0) {
    fprintf(stderr, "Failed to create_instance\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  free_link_list(&apilinks);

  return ret;
}
