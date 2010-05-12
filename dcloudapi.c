#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include "dcloudapi.h"
#include "common.h"
#include "curl_action.h"

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
    dcloudprintf("Failed to parse %s XML\n", name);
    return -1;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    dcloudprintf("Failed to get the root element for %s\n", name);
    goto cleanup;
  }

  if (STRNEQ((const char *)root->name, name)) {
    dcloudprintf("Failed to get expected root element '%s', saw root element '%s'\n",
		 name, root->name);
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    dcloudprintf("Failed to initialize XPath context for %s\n", name);
    goto cleanup;
  }

  if (cb(root->children, ctxt, data) < 0) {
    dcloudprintf("Failed XML parse callback\n");
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
  struct deltacloud_link **links = (struct deltacloud_link **)data;
  char *href = NULL, *rel = NULL;
  int listret;
  int ret = -1;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "link")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      rel = (char *)xmlGetProp(cur, BAD_CAST "rel");

      if (href == NULL) {
	dcloudprintf("Did not see href XML property\n");
	goto cleanup;
      }
      if (rel == NULL) {
	dcloudprintf("Did not see rel XML property\n");
	SAFE_FREE(href);
	goto cleanup;
      }

      listret = add_to_link_list(links, href, rel);
      SAFE_FREE(href);
      SAFE_FREE(rel);
      if (listret < 0) {
	dcloudprintf("Failed to add new link to list\n");
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

int deltacloud_initialize(struct deltacloud_api *api, char *url, char *user,
			  char *password)
{
  char *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  memset(api, 0, sizeof(struct deltacloud_api));

  api->url = strdup(url);
  if (api->url == NULL) {
    dcloudprintf("Failed to allocate memory for URL\n");
    return DELTACLOUD_OOM_ERROR;
  }
  api->user = strdup(user);
  if (api->user == NULL) {
    dcloudprintf("Failed to allocate memory for user\n");
    ret = DELTACLOUD_OOM_ERROR;
    goto cleanup;
  }
  api->password = strdup(password);
  if (api->password == NULL) {
    dcloudprintf("Failed to allocate memory for password\n");
    ret = DELTACLOUD_OOM_ERROR;
    goto cleanup;
  }
  api->links = NULL;

  data = get_url(api->url, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get XML for API from %s\n", api->url);
    ret = DELTACLOUD_GET_URL_ERROR;
    goto cleanup;
  }

  if (parse_xml(data, "api", (void **)&api->links, parse_api_xml) < 0) {
    dcloudprintf("Failed to parse 'api' XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  if (ret < 0)
    deltacloud_free(api);
  return ret;
}

static char *getXPathString(const char *xpath, xmlXPathContextPtr ctxt)
{
  xmlXPathObjectPtr obj;
  xmlNodePtr relnode;
  char *ret;

  if ((ctxt == NULL) || (xpath == NULL)) {
    dcloudprintf("Invalid parameter to virXPathString\n");
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
    dcloudprintf("Failed to copy XPath string\n");

  return ret;
}

static struct deltacloud_address *parse_addresses_xml(xmlNodePtr instance,
						      xmlXPathContextPtr ctxt)
{
  xmlNodePtr oldnode, cur;
  struct deltacloud_address *addresses = NULL;
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
      if (address != NULL) {
	listret = add_to_address_list(&addresses, address);
	SAFE_FREE(address);
	if (listret < 0) {
	  dcloudprintf("Failed to add new address to list\n");
	  goto cleanup;
	}
      }
      /* address is allowed to be NULL, so skip it here */
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

static struct deltacloud_action *parse_actions_xml(xmlNodePtr instance)
{
  xmlNodePtr cur;
  struct deltacloud_action *actions = NULL;
  char *rel = NULL, *href = NULL;
  int failed = 1;
  int listret;

  cur = instance->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "link")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      rel = (char *)xmlGetProp(cur, BAD_CAST "rel");

      if (href == NULL) {
	dcloudprintf("Did not see href XML property\n");
	goto cleanup;
      }
      if (rel == NULL) {
	dcloudprintf("Did not see rel XML property\n");
	SAFE_FREE(href);
	goto cleanup;
      }

      listret = add_to_action_list(&actions, rel, href);
      SAFE_FREE(href);
      SAFE_FREE(rel);
      if (listret < 0) {
	dcloudprintf("Failed to add new action to list\n");
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

static int parse_hardware_profile_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data);
static int parse_instance_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			      void **data)
{
  struct deltacloud_instance **instances = (struct deltacloud_instance **)data;
  xmlNodePtr oldnode, instance_cur;
  char *href = NULL, *id = NULL, *name = NULL, *owner_id = NULL;
  char *image_href = NULL, *realm_href = NULL, *state = NULL;
  struct deltacloud_hardware_profile *hwp = NULL;
  struct deltacloud_action *actions = NULL;
  struct deltacloud_address *public_addresses = NULL;
  struct deltacloud_address *private_addresses = NULL;
  int listret;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "instance")) {

      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	dcloudprintf("Did not see href XML property\n");
	goto cleanup;
      }

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
	  else if (STREQ((const char *)instance_cur->name, "realm"))
	    realm_href = (char *)xmlGetProp(cur, BAD_CAST "href");
	  else if (STREQ((const char *)instance_cur->name, "state"))
	    state = getXPathString("string(./state)", ctxt);
	  else if (STREQ((const char *)instance_cur->name, "hardware-profile"))
	    parse_hardware_profile_xml(instance_cur, ctxt, (void **)&hwp);
	  else if (STREQ((const char *)instance_cur->name, "actions"))
	    actions = parse_actions_xml(instance_cur);
	  else if (STREQ((const char *)instance_cur->name, "public-addresses"))
	    public_addresses = parse_addresses_xml(instance_cur, ctxt);
	  else if (STREQ((const char *)instance_cur->name, "private-addresses"))
	    private_addresses = parse_addresses_xml(instance_cur, ctxt);
	}

	instance_cur = instance_cur->next;
      }
      listret = add_to_instance_list(instances, href, id, name, owner_id,
				     image_href, realm_href, state, hwp,
				     actions, public_addresses,
				     private_addresses);
      SAFE_FREE(id);
      SAFE_FREE(name);
      SAFE_FREE(owner_id);
      SAFE_FREE(image_href);
      SAFE_FREE(realm_href);
      SAFE_FREE(state);
      SAFE_FREE(href);
      free_address_list(&public_addresses);
      free_address_list(&private_addresses);
      free_action_list(&actions);
      deltacloud_free_hardware_profile_list(&hwp);
      if (listret < 0) {
	dcloudprintf("Failed to add new instance to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_instance_list(instances);
  ctxt->node = oldnode;

  return ret;
}

static int parse_one_instance(const char *data,
			      struct deltacloud_instance *newinstance)
{
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = -1;
  xmlXPathContextPtr ctxt = NULL;
  struct deltacloud_instance *tmpinstance = NULL;

  xml = xmlReadDoc(BAD_CAST data, "instance.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    dcloudprintf("Failed to parse instance XML\n");
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    dcloudprintf("Failed to get the instance root element\n");
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    dcloudprintf("Failed to initialize XPath context for instance\n");
    goto cleanup;
  }

  if (parse_instance_xml(root, ctxt, (void **)&tmpinstance) < 0) {
    dcloudprintf("Failed to parse instance XML\n");
    goto cleanup;
  }

  if (copy_instance(newinstance, tmpinstance) < 0) {
    dcloudprintf("Failed to copy instance structure\n");
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_instance_list(&tmpinstance);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);

  return ret;
}

int deltacloud_get_instances(struct deltacloud_api *api,
			     struct deltacloud_instance **instances)
{
  struct deltacloud_link *thislink;
  char *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  thislink = find_by_rel_in_link_list(&api->links, "instances");
  if (thislink == NULL) {
    dcloudprintf("Failed to find the link for 'instances'\n");
    return DELTACLOUD_URL_DOES_NOT_EXIST;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for instances from %s\n",
		 thislink->href);
    return DELTACLOUD_GET_URL_ERROR;
  }

  *instances = NULL;
  if (parse_xml(data, "instances", (void **)instances, parse_instance_xml) < 0) {
    dcloudprintf("Failed to parse 'instances' XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_instance_by_id(struct deltacloud_api *api, const char *id,
				  struct deltacloud_instance *instance)
{
  char *url, *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  if (asprintf(&url, "%s/instances/%s", api->url, id) < 0) {
    dcloudprintf("Failed to allocate memory for URL\n");
    return DELTACLOUD_OOM_ERROR;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for instance from %s\n", url);
    ret = DELTACLOUD_GET_URL_ERROR;
    goto cleanup;
  }

  if (parse_one_instance(data, instance) < 0) {
    dcloudprintf("Failed to parse the 'instance' XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  SAFE_FREE(url);

  return ret;
}

int deltacloud_get_instance_by_name(struct deltacloud_api *api,
				    const char *name,
				    struct deltacloud_instance *instance)
{
  struct deltacloud_link *thislink;
  struct deltacloud_instance *instances = NULL;
  struct deltacloud_instance *thisinst;
  char *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  thislink = find_by_rel_in_link_list(&api->links, "instances");
  if (thislink == NULL) {
    dcloudprintf("Failed to find the link for 'instances'\n");
    return DELTACLOUD_URL_DOES_NOT_EXIST;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for instances from %s\n",
		 thislink->href);
    return DELTACLOUD_GET_URL_ERROR;
  }

  if (parse_xml(data, "instances", (void **)&instances, parse_instance_xml) < 0) {
    dcloudprintf("Failed to parse 'instances' XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  thisinst = find_by_name_in_instance_list(&instances, name);
  if (thisinst == NULL) {
    dcloudprintf("Failed to find instance '%s'\n", name);
    ret = DELTACLOUD_FIND_ERROR;
    goto cleanup;
  }

  if (copy_instance(instance, thisinst) < 0) {
    dcloudprintf("Failed to copy instance structure\n");
    ret = DELTACLOUD_OOM_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_instance_list(&instances);
  SAFE_FREE(data);

  return ret;
}

static int parse_realm_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			   void **data)
{
  struct deltacloud_realm **realms = (struct deltacloud_realm **)data;
  xmlNodePtr oldnode, realm_cur;
  int ret = -1;
  char *href = NULL, *id = NULL, *name = NULL, *state = NULL, *limit = NULL;
  int listret;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "realm")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	dcloudprintf("Did not see href XML property\n");
	goto cleanup;
      }

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
      SAFE_FREE(href);
      SAFE_FREE(id);
      SAFE_FREE(name);
      SAFE_FREE(state);
      SAFE_FREE(limit);
      if (listret < 0) {
	dcloudprintf("Failed to add new realm to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_realm_list(realms);
  ctxt->node = oldnode;

  return ret;
}

int deltacloud_get_realms(struct deltacloud_api *api,
			  struct deltacloud_realm **realms)
{
  struct deltacloud_link *thislink;
  char *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  thislink = find_by_rel_in_link_list(&api->links, "realms");
  if (thislink == NULL) {
    dcloudprintf("Failed to find the link for 'realms'\n");
    return DELTACLOUD_URL_DOES_NOT_EXIST;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for realms from %s\n", thislink->href);
    return DELTACLOUD_GET_URL_ERROR;
  }

  *realms = NULL;
  if (parse_xml(data, "realms", (void **)realms, parse_realm_xml) < 0) {
    dcloudprintf("Failed to parse 'realms' XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_realm_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_realm *realm)
{
  char *url, *data;
  struct deltacloud_realm *tmprealm = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = DELTACLOUD_UNKNOWN_ERROR;
  xmlXPathContextPtr ctxt = NULL;

  if (asprintf(&url, "%s/realms/%s", api->url, id) < 0) {
    dcloudprintf("Failed to allocate memory for URL\n");
    return DELTACLOUD_OOM_ERROR;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for realm from %s\n", url);
    ret = DELTACLOUD_GET_URL_ERROR;
    goto cleanup;
  }

  xml = xmlReadDoc(BAD_CAST data, "realm.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    dcloudprintf("Failed to parse realm XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    dcloudprintf("Failed to get the realm root element\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    dcloudprintf("Failed to initialize XPath context for realm\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  if (parse_realm_xml(root, ctxt, (void **)&tmprealm) < 0) {
    dcloudprintf("Failed to parse realm XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  if (copy_realm(realm, tmprealm) < 0) {
    dcloudprintf("Failed to copy realm structure\n");
    ret = DELTACLOUD_OOM_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_realm_list(&tmprealm);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  SAFE_FREE(data);
  SAFE_FREE(url);

  return ret;
}

static struct deltacloud_property_range *parse_hardware_profile_ranges(xmlNodePtr property)
{
  xmlNodePtr prop_cur;
  struct deltacloud_property_range *ranges = NULL;
  struct deltacloud_property_range *ret = NULL;
  char *first = NULL, *last = NULL;
  int listret;

  prop_cur = property->children;

  while (prop_cur != NULL) {
    if (prop_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)prop_cur->name, "range")) {

      first = (char *)xmlGetProp(prop_cur, BAD_CAST "first");
      last = (char *)xmlGetProp(prop_cur, BAD_CAST "last");

      listret = add_to_range_list(&ranges, first, last);
      SAFE_FREE(first);
      SAFE_FREE(last);
      if (listret < 0) {
	dcloudprintf("Failed to add range to list\n");
	free_range_list(&ranges);
	goto cleanup;
      }
    }

    prop_cur = prop_cur->next;
  }

  ret = ranges;

 cleanup:
  return ret;
}

static struct deltacloud_property_enum *parse_hardware_profile_enums(xmlNodePtr property)
{
  xmlNodePtr prop_cur, enum_cur;
  struct deltacloud_property_enum *enums = NULL;
  struct deltacloud_property_enum *ret = NULL;
  char *enum_value = NULL;
  int listret;

  prop_cur = property->children;

  while (prop_cur != NULL) {
    if (prop_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)prop_cur->name, "enum")) {

      enum_cur = prop_cur->children;
      while (enum_cur != NULL) {
	if (enum_cur->type == XML_ELEMENT_NODE &&
	    STREQ((const char *)enum_cur->name, "entry")) {
	  enum_value = (char *)xmlGetProp(enum_cur, BAD_CAST "value");

	  listret = add_to_enum_list(&enums, enum_value);
	  SAFE_FREE(enum_value);
	  if (listret < 0) {
	    dcloudprintf("Failed to add enum to list\n");
	    free_enum_list(&enums);
	    goto cleanup;
	  }
	}

	enum_cur = enum_cur->next;
      }
    }

    prop_cur = prop_cur->next;
  }

  ret = enums;

 cleanup:
  return ret;
}

static struct deltacloud_property_param *parse_hardware_profile_params(xmlNodePtr property)
{
  xmlNodePtr prop_cur;
  struct deltacloud_property_param *params = NULL;
  struct deltacloud_property_param *ret = NULL;
  char *param_href = NULL, *param_method = NULL, *param_name = NULL;
  char *param_operation = NULL;
  int listret;

  prop_cur = property->children;

  while (prop_cur != NULL) {
    if (prop_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)prop_cur->name, "param")) {

      param_href = (char *)xmlGetProp(prop_cur, BAD_CAST "href");
      param_method = (char *)xmlGetProp(prop_cur, BAD_CAST "method");
      param_name = (char *)xmlGetProp(prop_cur, BAD_CAST "name");
      param_operation = (char *)xmlGetProp(prop_cur, BAD_CAST "operation");

      listret = add_to_param_list(&params, param_href,
				  param_method, param_name,
				  param_operation);
      SAFE_FREE(param_href);
      SAFE_FREE(param_method);
      SAFE_FREE(param_name);
      SAFE_FREE(param_operation);
      if (listret < 0) {
	dcloudprintf("Failed to add new property to list\n");
	free_param_list(&params);
	goto cleanup;
      }
    }
    prop_cur = prop_cur->next;
  }

  ret = params;

 cleanup:
  return ret;
}

static struct deltacloud_property *parse_hardware_profile_properties(xmlNodePtr hwp)
{
  xmlNodePtr profile_cur;
  struct deltacloud_property *props = NULL;
  struct deltacloud_property *ret = NULL;
  struct deltacloud_property_param *params = NULL;
  struct deltacloud_property_enum *enums = NULL;
  struct deltacloud_property_range *ranges = NULL;
  char *kind = NULL, *name = NULL, *unit = NULL, *value = NULL;
  int listret;

  profile_cur = hwp->children;

  while (profile_cur != NULL) {
    if (profile_cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)profile_cur->name, "property")) {
      kind = (char *)xmlGetProp(profile_cur, BAD_CAST "kind");
      name = (char *)xmlGetProp(profile_cur, BAD_CAST "name");
      unit = (char *)xmlGetProp(profile_cur, BAD_CAST "unit");
      value = (char *)xmlGetProp(profile_cur, BAD_CAST "value");

      params = parse_hardware_profile_params(profile_cur);
      enums = parse_hardware_profile_enums(profile_cur);
      ranges = parse_hardware_profile_ranges(profile_cur);

      listret = add_to_property_list(&props, kind, name, unit, value, params,
				     enums, ranges);
      SAFE_FREE(kind);
      SAFE_FREE(name);
      SAFE_FREE(unit);
      SAFE_FREE(value);
      free_param_list(&params);
      free_enum_list(&enums);
      free_range_list(&ranges);
      if (listret < 0) {
	dcloudprintf("Failed to add new property to list\n");
	free_property_list(&props);
	goto cleanup;
      }
    }

    profile_cur = profile_cur->next;
  }

  ret = props;

 cleanup:
  return ret;
}

static int parse_hardware_profile_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data)
{
  struct deltacloud_hardware_profile **profiles = (struct deltacloud_hardware_profile **)data;
  struct deltacloud_property *props = NULL;
  char *href = NULL, *id = NULL;
  xmlNodePtr oldnode;
  int ret = -1;
  int listret;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "hardware-profile")) {
      ctxt->node = cur;

      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	dcloudprintf("Did not see href XML property\n");
	goto cleanup;
      }

      id = getXPathString("string(./id)", ctxt);

      props = parse_hardware_profile_properties(cur);

      listret = add_to_hardware_profile_list(profiles, id, href, props);
      SAFE_FREE(id);
      SAFE_FREE(href);
      free_property_list(&props);
      if (listret < 0) {
	dcloudprintf("Failed to add new hardware profile to list\n");
	goto cleanup;
      }
    }

    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_hardware_profile_list(profiles);

  return ret;
}

int deltacloud_get_hardware_profiles(struct deltacloud_api *api,
				     struct deltacloud_hardware_profile **profiles)
{
  struct deltacloud_link *thislink;
  char *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  thislink = find_by_rel_in_link_list(&api->links, "hardware_profiles");
  if (thislink == NULL) {
    dcloudprintf("Failed to find the link for 'hardware_profiles'\n");
    return DELTACLOUD_URL_DOES_NOT_EXIST;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for flavors from %s\n", thislink->href);
    return DELTACLOUD_GET_URL_ERROR;
  }

  *profiles = NULL;
  if (parse_xml(data, "hardware-profiles", (void **)profiles,
		parse_hardware_profile_xml) < 0) {
    dcloudprintf("Failed to parse 'hardware-profiles' XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_hardware_profile_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_hardware_profile *profile)
{
  char *url, *data;
  struct deltacloud_hardware_profile *tmpprofile = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = DELTACLOUD_UNKNOWN_ERROR;
  xmlXPathContextPtr ctxt = NULL;

  if (asprintf(&url, "%s/hardware_profiles/%s", api->url, id) < 0) {
    dcloudprintf("Failed to allocate memory for URL\n");
    return DELTACLOUD_OOM_ERROR;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for image from %s\n", url);
    ret = DELTACLOUD_GET_URL_ERROR;
    goto cleanup;
  }

  xml = xmlReadDoc(BAD_CAST data, "hardware_profile.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    dcloudprintf("Failed to parse hardware_profile XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    dcloudprintf("Failed to get the image root element\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    dcloudprintf("Failed to initialize XPath context for image\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  if (parse_hardware_profile_xml(root, ctxt, (void **)&tmpprofile) < 0) {
    dcloudprintf("Failed to parse hardware profile XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  if (copy_hardware_profile(profile, tmpprofile) < 0) {
    dcloudprintf("Failed to copy hardware profile structure\n");
    ret = DELTACLOUD_OOM_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_hardware_profile_list(&tmpprofile);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  SAFE_FREE(data);
  SAFE_FREE(url);

  return ret;
}

static int parse_image_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			   void **data)
{
  struct deltacloud_image **images = (struct deltacloud_image **)data;
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
      if (href == NULL) {
	dcloudprintf("Did not see href XML property\n");
	goto cleanup;
      }

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
      SAFE_FREE(href);
      SAFE_FREE(id);
      SAFE_FREE(description);
      SAFE_FREE(architecture);
      SAFE_FREE(owner_id);
      SAFE_FREE(name);
      if (listret < 0) {
	dcloudprintf("Failed to add new image to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_image_list(images);

  return ret;
}

int deltacloud_get_images(struct deltacloud_api *api,
			  struct deltacloud_image **images)
{
  struct deltacloud_link *thislink;
  char *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  thislink = find_by_rel_in_link_list(&api->links, "images");
  if (thislink == NULL) {
    dcloudprintf("Failed to find the link for 'images'\n");
    return DELTACLOUD_URL_DOES_NOT_EXIST;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for images from %s\n", thislink->href);
    return DELTACLOUD_GET_URL_ERROR;
  }

  *images = NULL;
  if (parse_xml(data, "images", (void **)images, parse_image_xml) < 0) {
    dcloudprintf("Failed to parse 'images' XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_image_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_image *image)
{
  char *url, *data;
  struct deltacloud_image *tmpimage = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = DELTACLOUD_UNKNOWN_ERROR;
  xmlXPathContextPtr ctxt = NULL;

  if (asprintf(&url, "%s/images/%s", api->url, id) < 0) {
    dcloudprintf("Failed to allocate memory for URL\n");
    return DELTACLOUD_OOM_ERROR;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for image from %s\n", url);
    ret = DELTACLOUD_GET_URL_ERROR;
    goto cleanup;
  }

  xml = xmlReadDoc(BAD_CAST data, "image.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    dcloudprintf("Failed to parse image XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    dcloudprintf("Failed to get the image root element\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    dcloudprintf("Failed to initialize XPath context for image\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  if (parse_image_xml(root, ctxt, (void **)&tmpimage) < 0) {
    dcloudprintf("Failed to parse image XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  if (copy_image(image, tmpimage) < 0) {
    dcloudprintf("Failed to copy image structure\n");
    ret = DELTACLOUD_OOM_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_image_list(&tmpimage);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  SAFE_FREE(data);
  SAFE_FREE(url);

  return ret;
}

static int parse_instance_state_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void **data)
{
  struct deltacloud_instance_state **instance_states = (struct deltacloud_instance_state **)data;
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
	  SAFE_FREE(action);
	  SAFE_FREE(to);
	}
	state_cur = state_cur->next;
      }
      listret = add_to_instance_state_list(instance_states, name, transitions);
      SAFE_FREE(name);
      free_transition_list(&transitions);
      if (listret < 0) {
	dcloudprintf("Failed to add new instance_state to list\n");
	goto cleanup;
      }
      transitions = NULL;
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_instance_state_list(instance_states);

  return ret;
}

int deltacloud_get_instance_states(struct deltacloud_api *api,
				   struct deltacloud_instance_state **instance_states)
{
  struct deltacloud_link *thislink;
  char *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  thislink = find_by_rel_in_link_list(&api->links, "instance_states");
  if (thislink == NULL) {
    dcloudprintf("Failed to find the link for 'instance-states'\n");
    return DELTACLOUD_URL_DOES_NOT_EXIST;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for instance_states from %s\n",
		 thislink->href);
    return DELTACLOUD_GET_URL_ERROR;
  }

  *instance_states = NULL;
  if (parse_xml(data, "states", (void **)instance_states, parse_instance_state_xml) < 0) {
    dcloudprintf("Failed to parse 'instance_states' XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_instance_state_by_name(struct deltacloud_api *api,
					  const char *name,
					  struct deltacloud_instance_state *instance_state)
{
  struct deltacloud_instance_state *statelist = NULL;
  struct deltacloud_instance_state *found;
  int instance_ret;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  instance_ret = deltacloud_get_instance_states(api, &statelist);
  if (instance_ret < 0) {
    dcloudprintf("Failed to get instance_states\n");
    return instance_ret;
  }

  found = find_by_name_in_instance_state_list(&statelist, name);
  if (found == NULL) {
    dcloudprintf("Failed to find %s in instance_state list\n", name);
    ret = DELTACLOUD_URL_DOES_NOT_EXIST;
    goto cleanup;
  }
  if (copy_instance_state(instance_state, found) < 0) {
    dcloudprintf("Failed to copy instance_state structure\n");
    ret = DELTACLOUD_OOM_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_instance_state_list(&statelist);
  return ret;
}

static int parse_storage_volume_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				    void **data)
{
  struct deltacloud_storage_volume **storage_volumes = (struct deltacloud_storage_volume **)data;
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
      if (href == NULL) {
	dcloudprintf("Did not see href XML property\n");
	goto cleanup;
      }

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
      SAFE_FREE(id);
      SAFE_FREE(created);
      SAFE_FREE(state);
      SAFE_FREE(capacity);
      SAFE_FREE(device);
      SAFE_FREE(instance_href);
      SAFE_FREE(href);
      if (listret < 0) {
	dcloudprintf("Failed to add new storage_volume to list\n");
	goto cleanup;
      }
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_storage_volume_list(storage_volumes);

  return ret;
}

int deltacloud_get_storage_volumes(struct deltacloud_api *api,
				   struct deltacloud_storage_volume **storage_volumes)
{
  struct deltacloud_link *thislink;
  char *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  thislink = find_by_rel_in_link_list(&api->links, "storage_volumes");
  if (thislink == NULL) {
    dcloudprintf("Failed to find the link for 'storage_volumes'\n");
    return DELTACLOUD_URL_DOES_NOT_EXIST;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for storage_volumes from %s\n",
		 thislink->href);
    return DELTACLOUD_GET_URL_ERROR;
  }

  *storage_volumes = NULL;
  if (parse_xml(data, "storage-volumes", (void **)storage_volumes, parse_storage_volume_xml) < 0) {
    dcloudprintf("Failed to parse 'storage_volumes' XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_storage_volume_by_id(struct deltacloud_api *api,
					const char *id,
					struct deltacloud_storage_volume *storage_volume)
{
  char *url, *data;
  struct deltacloud_storage_volume *tmpstorage_volume = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = DELTACLOUD_UNKNOWN_ERROR;
  xmlXPathContextPtr ctxt = NULL;

  if (asprintf(&url, "%s/storage_volumes/%s", api->url, id) < 0) {
    dcloudprintf("Failed to allocate memory for URL\n");
    return DELTACLOUD_OOM_ERROR;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for storage_volume from %s\n", url);
    ret = DELTACLOUD_GET_URL_ERROR;
    goto cleanup;
  }

  xml = xmlReadDoc(BAD_CAST data, "storage_volume.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    dcloudprintf("Failed to parse storage_volume XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    dcloudprintf("Failed to get the storage_volume root element\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    dcloudprintf("Failed to initialize XPath context for storage_volume\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  if (parse_storage_volume_xml(root, ctxt, (void **)&tmpstorage_volume) < 0) {
    dcloudprintf("Failed to parse storage_volume XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  if (copy_storage_volume(storage_volume, tmpstorage_volume) < 0) {
    dcloudprintf("Failed to copy storage_volume structure\n");
    ret = DELTACLOUD_OOM_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_storage_volume_list(&tmpstorage_volume);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  SAFE_FREE(data);
  SAFE_FREE(url);

  return ret;
}

static int parse_storage_snapshot_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				      void **data)
{
  struct deltacloud_storage_snapshot **storage_snapshots = (struct deltacloud_storage_snapshot **)data;
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
      if (href == NULL) {
	dcloudprintf("Did not see href XML property\n");
	goto cleanup;
      }

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
      SAFE_FREE(id);
      SAFE_FREE(created);
      SAFE_FREE(state);
      SAFE_FREE(storage_volume_href);
      SAFE_FREE(href);
      if (listret < 0) {
	dcloudprintf("Failed to add new storage_snapshot to list\n");
	goto cleanup;
      }
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

int deltacloud_get_storage_snapshots(struct deltacloud_api *api,
				     struct deltacloud_storage_snapshot **storage_snapshots)
{
  struct deltacloud_link *thislink;
  char *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  thislink = find_by_rel_in_link_list(&api->links, "storage_snapshots");
  if (thislink == NULL) {
    dcloudprintf("Failed to find the link for 'storage_snapshots'\n");
    return DELTACLOUD_URL_DOES_NOT_EXIST;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for storage_snapshots from %s\n",
		 thislink->href);
    return DELTACLOUD_GET_URL_ERROR;
  }

  *storage_snapshots = NULL;
  if (parse_xml(data, "storage-snapshots", (void **)storage_snapshots, parse_storage_snapshot_xml) < 0) {
    dcloudprintf("Failed to parse 'storage_snapshots' XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_storage_snapshot_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_storage_snapshot *storage_snapshot)
{
  char *url, *data;
  struct deltacloud_storage_snapshot *tmpstorage_snapshot = NULL;
  xmlDocPtr xml = NULL;
  xmlNodePtr root;
  int ret = DELTACLOUD_UNKNOWN_ERROR;
  xmlXPathContextPtr ctxt = NULL;

  if (asprintf(&url, "%s/storage_snapshots/%s", api->url, id) < 0) {
    dcloudprintf("Failed to allocate memory for URL\n");
    return DELTACLOUD_OOM_ERROR;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    dcloudprintf("Failed to get the XML for storage_snapshot from %s\n", url);
    ret = DELTACLOUD_GET_URL_ERROR;
    goto cleanup;
  }

  xml = xmlReadDoc(BAD_CAST data, "storage_snapshot.xml", NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    dcloudprintf("Failed to parse storage_snapshot XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    dcloudprintf("Failed to get the storage_snapshot root element\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    dcloudprintf("Failed to initialize XPath context for storage_snapshot\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  if (parse_storage_snapshot_xml(root, ctxt, (void **)&tmpstorage_snapshot) < 0) {
    dcloudprintf("Failed to parse storage_snapshot XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  if (copy_storage_snapshot(storage_snapshot, tmpstorage_snapshot) < 0) {
    dcloudprintf("Failed to copy storage_snapshot structure\n");
    ret = DELTACLOUD_OOM_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_storage_snapshot_list(&tmpstorage_snapshot);
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  if (xml)
    xmlFreeDoc(xml);
  SAFE_FREE(data);
  SAFE_FREE(url);

  return ret;
}

int deltacloud_create_instance(struct deltacloud_api *api, const char *image_id,
			       const char *name, const char *realm_id,
			       const char *hardware_profile,
			       struct deltacloud_instance *inst)
{
  struct deltacloud_link *thislink;
  char *data, *params;
  size_t param_size;
  FILE *paramfp;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  if (image_id == NULL) {
    dcloudprintf("Failed create_instance: image ID cannot be NULL\n");
    return DELTACLOUD_INVALID_IMAGE_ERROR;
  }

  thislink = find_by_rel_in_link_list(&api->links, "instances");
  if (thislink == NULL) {
    dcloudprintf("Failed to find the link for 'instances'\n");
    return DELTACLOUD_URL_DOES_NOT_EXIST;
  }

  paramfp = open_memstream(&params, &param_size);
  if (paramfp == NULL) {
    dcloudprintf("Failed to allocate memory for parameters\n");
    return DELTACLOUD_OOM_ERROR;
  }

  fprintf(paramfp, "image_id=%s", image_id);
  if (name != NULL)
    fprintf(paramfp, "&name=%s", name);
  if (realm_id != NULL)
    fprintf(paramfp, "&realm_id=%s", realm_id);
  if (hardware_profile != NULL)
    fprintf(paramfp, "&hwp_id=%s", hardware_profile);
  fclose(paramfp);

  data = post_url(thislink->href, api->user, api->password, params, param_size);
  SAFE_FREE(params);
  if (data == NULL) {
    dcloudprintf("Failed to post the XML for create_instance to %s\n",
		 thislink->href);
    return DELTACLOUD_POST_URL_ERROR;
  }

  if (inst != NULL) {
    if (parse_one_instance(data, inst) < 0) {
      dcloudprintf("Failed to parse instance XML\n");
      ret = DELTACLOUD_XML_PARSE_ERROR;
      goto cleanup;
    }
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

static int instance_action(struct deltacloud_api *api,
			   struct deltacloud_instance *instance,
			   const char *action_name)
{
  struct deltacloud_action *act;
  char *data;
  int ret = DELTACLOUD_UNKNOWN_ERROR;

  act = find_by_rel_in_action_list(&instance->actions, action_name);
  if (act == NULL) {
    dcloudprintf("Failed to find the link for '%s'\n", action_name);
    return DELTACLOUD_URL_DOES_NOT_EXIST;
  }

  data = post_url(act->href, api->user, api->password, NULL, 0);
  if (data == NULL) {
    dcloudprintf("Failed to post the XML for action %s to %s\n",
		 action_name, act->href);
    return DELTACLOUD_POST_URL_ERROR;
  }

  deltacloud_free_instance(instance);

  if (parse_one_instance(data, instance) < 0) {
    dcloudprintf("Failed to parse instance XML\n");
    ret = DELTACLOUD_XML_PARSE_ERROR;
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_instance_stop(struct deltacloud_api *api,
			     struct deltacloud_instance *instance)
{
  return instance_action(api, instance, "stop");
}

int deltacloud_instance_reboot(struct deltacloud_api *api,
			       struct deltacloud_instance *instance)
{
  return instance_action(api, instance, "reboot");
}

int deltacloud_instance_start(struct deltacloud_api *api,
			      struct deltacloud_instance *instance)
{
  return instance_action(api, instance, "start");
}

int deltacloud_instance_destroy(struct deltacloud_api *api,
				struct deltacloud_instance *instance)
{
  /* in deltacloud the destroy action is a DELETE method, so we need
   * to use a different implementation
   */
  return delete_url(instance->href, api->user, api->password);
}

struct deltacloud_error_entry {
  int number;
  const char *desc;
};

static const struct deltacloud_error_entry errors[] = {
  { DELTACLOUD_UNKNOWN_ERROR, "Unknown error" },
  { DELTACLOUD_GET_URL_ERROR, "Failed to get url" },
  { DELTACLOUD_POST_URL_ERROR, "Failed to post data to url" },
  { DELTACLOUD_XML_PARSE_ERROR, "Failed to parse the XML" },
  { DELTACLOUD_URL_DOES_NOT_EXIST, "Failed to find required URL" },
  { DELTACLOUD_OOM_ERROR, "Out of memory" },
  { DELTACLOUD_INVALID_IMAGE_ERROR, "Invalid image ID" },
  { DELTACLOUD_FIND_ERROR, "Failed to find requested information" },
  { 0, NULL },
};

const char *deltacloud_strerror(int error)
{
  const struct deltacloud_error_entry *p;

  for (p = errors; p->desc != NULL; p++) {
    if (p->number == error)
      return p->desc;
  }

  return "Unknown Error";
}

void deltacloud_free(struct deltacloud_api *api)
{
  free_link_list(&api->links);
  SAFE_FREE(api->user);
  SAFE_FREE(api->password);
  SAFE_FREE(api->url);
  memset(api, 0, sizeof(struct deltacloud_api));
}
