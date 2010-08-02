#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <curl/curl.h>
#include <pthread.h>
#include "libdeltacloud.h"
#include "common.h"
#include "curl_action.h"

/***************** ERROR HANDLING ROUTINES ***************************/
/* On fist initialization of the library we setup a per-thread local
 * variable to hold errors.  If one of the API calls subsequently fails,
 * then we set the per-thread variable with details of the failure.
 */
static int tlsinitialized = 0;
static pthread_key_t deltacloud_last_error;

static void deltacloud_error_free_data(void *data)
{
  struct deltacloud_error *err = data;

  if (err == NULL)
    return;

  SAFE_FREE(err->details);
  SAFE_FREE(err);
}

struct deltacloud_error *deltacloud_get_last_error(void)
{
  return pthread_getspecific(deltacloud_last_error);
}

const char *deltacloud_get_last_error_string(void)
{
  struct deltacloud_error *last;

  last = deltacloud_get_last_error();
  if (last != NULL)
    return last->details;
  return NULL;
}

static void set_error(int errnum, const char *details)
{
  struct deltacloud_error *err;
  struct deltacloud_error *last;

  err = (struct deltacloud_error *)malloc(sizeof(struct deltacloud_error));
  if (err == NULL) {
    /* if we failed to allocate memory here, there's not a lot we can do */
    dcloudprintf("Failed to allocate memory in an error path; error information will be unreliable!\n");
    return;
  }
  memset(err, 0, sizeof(struct deltacloud_error));

  err->error_num = errnum;
  err->details = strdup(details);

  dcloudprintf("%s\n", err->details);

  last = pthread_getspecific(deltacloud_last_error);
  if (last != NULL)
    deltacloud_error_free_data(last);

  pthread_setspecific(deltacloud_last_error, err);
}

static void get_url_error(const char *type, const char *url)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Failed to get the XML for %s from %s", type, url) < 0) {
    tmp = "Failed to get the XML";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_GET_URL_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void post_url_error(const char *type, const char *url)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Failed to post the XML for %s from %s", type, url) < 0) {
    tmp = "Failed to post the XML";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_POST_URL_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void xml_error(const char *name, const char *type, const char *details)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "%s for %s: %s", type, name, details) < 0) {
    tmp = "Failed parsing XML";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_XML_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void link_error(const char *name)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Failed to find the link for '%s'", name) < 0) {
    tmp = "Failed to find the link";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_URL_DOES_NOT_EXIST_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void find_by_name_error(const char *name, const char *type)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Failed to find '%s' in '%s' list", name, type) < 0) {
    tmp = "Failed to find the link";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_NAME_NOT_FOUND_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

static void oom_error(void)
{
  set_error(DELTACLOUD_OOM_ERROR, "Failed to allocate memory");
}

static void invalid_argument_error(const char *details)
{
  set_error(DELTACLOUD_INVALID_ARGUMENT_ERROR, details);
}

/********************** XML PARSING *********************************/
static char *getXPathString(const char *xpath, xmlXPathContextPtr ctxt)
{
  xmlXPathObjectPtr obj;
  xmlNodePtr relnode;
  char *ret;

  if ((ctxt == NULL) || (xpath == NULL))
    return NULL;

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

  return ret;
}

typedef int (*parse_xml_callback)(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data);

static int parse_xml(const char *xml_string, const char *name, void **data,
		     parse_xml_callback cb, int multiple)
{
  xmlDocPtr xml;
  xmlNodePtr root;
  xmlXPathContextPtr ctxt = NULL;
  int ret = -1;
  int rc;
  xmlErrorPtr last;
  char *msg;

  xml = xmlReadDoc(BAD_CAST xml_string, name, NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    last = xmlGetLastError();
    if (last != NULL)
      msg = last->message;
    else
      msg = "unknown error";
    xml_error(name, "Failed to parse XML", msg);
    return -1;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    last = xmlGetLastError();
    if (last != NULL)
      msg = last->message;
    else
      msg = "unknown error";
    xml_error(name, "Failed to get the root element", msg);
    goto cleanup;
  }

  if (STRNEQ((const char *)root->name, name)) {
    xml_error(name, "Failed to get expected root element", (char *)root->name);
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    last = xmlGetLastError();
    if (last != NULL)
      msg = last->message;
    else
      msg = "unknown error";
    xml_error(name, "Failed to initialize XPath context", msg);
    goto cleanup;
  }

  /* if "multiple" is true, then the XML looks something like:
   * <instances> <instance> ... </instance> </instances>"
   * if "multiple" is false, then the XML looks something like:
   * <instance> ... </instance>
   */
  if (multiple)
    rc = cb(root->children, ctxt, data);
  else
    rc = cb(root, ctxt, data);

  if (rc < 0)
    /* the callbacks are expected to have set their own error */
    goto cleanup;

  ret = 0;

 cleanup:
  if (ctxt != NULL)
    xmlXPathFreeContext(ctxt);
  xmlFreeDoc(xml);

  return ret;
}

static int parse_error_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data)
{
  char **msg = (char **)data;
  xmlNodePtr oldnode;

  oldnode = ctxt->node;

  *msg = NULL;

  while (cur != NULL) {
    ctxt->node = cur;
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "message")) {
      *msg = getXPathString("string(.)", ctxt);
      break;
    }
    cur = cur->next;
  }

  if (*msg == NULL)
    *msg = strdup("Unknown error");

  ctxt->node = oldnode;

  return 0;
}

static int is_error_xml(const char *xml)
{
  return STRPREFIX(xml, "<error");
}
static void set_xml_error(const char *xml, int type)
{
  char *errmsg = NULL;

  if (parse_xml(xml, "error", (void **)&errmsg, parse_error_xml, 1) < 0)
    errmsg = strdup("Unknown error");

  set_error(type, errmsg);

  SAFE_FREE(errmsg);
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
      if (href == NULL) {
	xml_error("api", "Failed to parse XML", "did not see href property");
	goto cleanup;
      }

      rel = (char *)xmlGetProp(cur, BAD_CAST "rel");
      if (rel == NULL) {
	xml_error("api", "Failed to parse XML", "did not see rel property");
	SAFE_FREE(href);
	goto cleanup;
      }

      listret = add_to_link_list(links, href, rel);
      SAFE_FREE(href);
      SAFE_FREE(rel);
      if (listret < 0) {
	oom_error();
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
  char *data = NULL;
  int ret = -1;

  if (!tlsinitialized) {
    tlsinitialized = 1;
    if (pthread_key_create(&deltacloud_last_error, deltacloud_error_free_data) != 0) {
      dcloudprintf("Failed to initialize error handler\n");
      return -1;
    }
  }

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (url == NULL) {
    invalid_argument_error("URL argument cannot be NULL");
    return -1;
  }
  if (user == NULL) {
    invalid_argument_error("User cannot be NULL");
    return -1;
  }
  if (password == NULL) {
    invalid_argument_error("Password cannot be NULL");
    return -1;
  }

  memset(api, 0, sizeof(struct deltacloud_api));

  api->url = strdup(url);
  if (api->url == NULL) {
    oom_error();
    return -1;
  }
  api->user = strdup(user);
  if (api->user == NULL) {
    oom_error();
    goto cleanup;
  }
  api->password = strdup(password);
  if (api->password == NULL) {
    oom_error();
    goto cleanup;
  }
  api->links = NULL;

  data = get_url(api->url, api->user, api->password);
  if (data == NULL) {
    get_url_error("API", api->url);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml(data, "api", (void **)&api->links, parse_api_xml, 1) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  if (ret < 0)
    deltacloud_free(api);
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
	  oom_error();
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
      if (href == NULL) {
	xml_error("actions", "Failed to parse XML", "did not see href property");
	goto cleanup;
      }

      rel = (char *)xmlGetProp(cur, BAD_CAST "rel");
      if (rel == NULL) {
	xml_error("actions", "Failed to parse XML", "did not see rel property");
	SAFE_FREE(href);
	goto cleanup;
      }

      listret = add_to_action_list(&actions, rel, href);
      SAFE_FREE(href);
      SAFE_FREE(rel);
      if (listret < 0) {
	oom_error();
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
	xml_error("instance", "Failed to parse XML", "did not see rel property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("instance", "Failed to parse XML", "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;
      instance_cur = cur->children;
      while (instance_cur != NULL) {
	if (instance_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)instance_cur->name, "name"))
	    name = getXPathString("string(./name)", ctxt);
	  else if (STREQ((const char *)instance_cur->name, "owner_id"))
	    owner_id = getXPathString("string(./owner_id)", ctxt);
	  else if (STREQ((const char *)instance_cur->name, "image"))
	    image_href = (char *)xmlGetProp(cur, BAD_CAST "href");
	  else if (STREQ((const char *)instance_cur->name, "realm"))
	    realm_href = (char *)xmlGetProp(cur, BAD_CAST "href");
	  else if (STREQ((const char *)instance_cur->name, "state"))
	    state = getXPathString("string(./state)", ctxt);
	  else if (STREQ((const char *)instance_cur->name, "hardware_profile"))
	    parse_hardware_profile_xml(instance_cur, ctxt, (void **)&hwp);
	  else if (STREQ((const char *)instance_cur->name, "actions"))
	    actions = parse_actions_xml(instance_cur);
	  else if (STREQ((const char *)instance_cur->name, "public_addresses"))
	    public_addresses = parse_addresses_xml(instance_cur, ctxt);
	  else if (STREQ((const char *)instance_cur->name, "private_addresses"))
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
	oom_error();
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
  int ret = -1;
  struct deltacloud_instance *tmpinstance = NULL;

  if (parse_xml(data, "instance", (void **)&tmpinstance,
		parse_instance_xml, 0) < 0)
    goto cleanup;

  if (copy_instance(newinstance, tmpinstance) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_instance_list(&tmpinstance);

  return ret;
}

int deltacloud_get_instances(struct deltacloud_api *api,
			     struct deltacloud_instance **instances)
{
  struct deltacloud_link *thislink;
  char *data;
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (instances == NULL) {
    invalid_argument_error("Instances argument cannot be NULL");
    return -1;
  }

  thislink = find_by_rel_in_link_list(&api->links, "instances");
  if (thislink == NULL) {
    link_error("instances");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    get_url_error("instances", thislink->href);
    return -1;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  *instances = NULL;
  if (parse_xml(data, "instances", (void **)instances,
		parse_instance_xml, 1) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_instance_by_id(struct deltacloud_api *api, const char *id,
				  struct deltacloud_instance *instance)
{
  char *url = NULL;
  char *data = NULL;
  char *safeid;
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (id == NULL) {
    invalid_argument_error("Instance ID argument cannot be NULL");
    return -1;
  }
  if (instance == NULL) {
    invalid_argument_error("Instance argument cannot be NULL");
    return -1;
  }

  safeid = curl_escape(id, 0);
  if (safeid == NULL) {
    oom_error();
    return -1;
  }

  if (asprintf(&url, "%s/instances/%s", api->url, safeid) < 0) {
    oom_error();
    goto cleanup;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    get_url_error("instance", url);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  memset(instance, 0, sizeof(struct deltacloud_instance));
  if (parse_one_instance(data, instance) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  SAFE_FREE(url);
  curl_free(safeid);

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
  int ret = -1;

  /* despite the fact that 'name' is an input from the user, we don't
   * need to escape it since we are never using it as a URL
   */

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (name == NULL) {
    invalid_argument_error("Instance name argument cannot be NULL");
    return -1;
  }
  if (instance == NULL) {
    invalid_argument_error("Instance argument cannot be NULL");
    return -1;
  }

  thislink = find_by_rel_in_link_list(&api->links, "instances");
  if (thislink == NULL) {
    link_error("instances");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    get_url_error("instances", thislink->href);
    return -1;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml(data, "instances", (void **)&instances,
		parse_instance_xml, 1) < 0)
    goto cleanup;

  thisinst = find_by_name_in_instance_list(&instances, name);
  if (thisinst == NULL) {
    find_by_name_error(name, "instances");
    goto cleanup;
  }

  if (copy_instance(instance, thisinst) < 0) {
    oom_error();
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
	xml_error("realm", "Failed to parse XML", "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("realm", "Failed to parse XML", "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;
      realm_cur = cur->children;
      while (realm_cur != NULL) {
	if (realm_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)realm_cur->name, "name"))
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
	oom_error();
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
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (realms == NULL) {
    invalid_argument_error("Realms argument cannot be NULL");
    return -1;
  }

  thislink = find_by_rel_in_link_list(&api->links, "realms");
  if (thislink == NULL) {
    link_error("realms");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    get_url_error("realms", thislink->href);
    return -1;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  *realms = NULL;
  if (parse_xml(data, "realms", (void **)realms, parse_realm_xml, 1) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_realm_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_realm *realm)
{
  char *url = NULL;
  char *data = NULL;
  char *safeid;
  struct deltacloud_realm *tmprealm = NULL;
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (id == NULL) {
    invalid_argument_error("Realm ID argument cannot be NULL");
    return -1;
  }
  if (realm == NULL) {
    invalid_argument_error("Realm argument cannot be NULL");
    return -1;
  }

  safeid = curl_escape(id, 0);
  if (safeid == NULL) {
    oom_error();
    return -1;
  }

  if (asprintf(&url, "%s/realms/%s", api->url, safeid) < 0) {
    oom_error();
    goto cleanup;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    get_url_error("realm", url);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml(data, "realm", (void **)&tmprealm, parse_realm_xml, 0) < 0)
    goto cleanup;

  if (copy_realm(realm, tmprealm) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_realm_list(&tmprealm);
  SAFE_FREE(data);
  SAFE_FREE(url);
  curl_free(safeid);

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
	oom_error();
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
	    oom_error();
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
	oom_error();
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
	oom_error();
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
	STREQ((const char *)cur->name, "hardware_profile")) {
      ctxt->node = cur;

      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("hardware_profile", "Failed to parse XML",
		  "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("hardware_profile", "Failed to parse XML",
		  "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      props = parse_hardware_profile_properties(cur);

      listret = add_to_hardware_profile_list(profiles, id, href, props);
      SAFE_FREE(id);
      SAFE_FREE(href);
      free_property_list(&props);
      if (listret < 0) {
	oom_error();
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
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (profiles == NULL) {
    invalid_argument_error("Hardware profiles argument cannot be NULL");
    return -1;
  }

  thislink = find_by_rel_in_link_list(&api->links, "hardware_profiles");
  if (thislink == NULL) {
    link_error("hardware_profiles");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    get_url_error("hardware_profiles", thislink->href);
    return -1;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  *profiles = NULL;
  if (parse_xml(data, "hardware_profiles", (void **)profiles,
		parse_hardware_profile_xml, 1) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_hardware_profile_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_hardware_profile *profile)
{
  char *url = NULL;
  char *data = NULL;
  char *safeid;
  struct deltacloud_hardware_profile *tmpprofile = NULL;
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (id == NULL) {
    invalid_argument_error("Hardware profile ID argument cannot be NULL");
    return -1;
  }
  if (profile == NULL) {
    invalid_argument_error("Hardware profile argument cannot be NULL");
    return -1;
  }

  safeid = curl_escape(id, 0);
  if (safeid == NULL) {
    oom_error();
    return -1;
  }

  if (asprintf(&url, "%s/hardware_profiles/%s", api->url, safeid) < 0) {
    oom_error();
    goto cleanup;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    get_url_error("image", url);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml(data, "hardware_profile", (void **)&tmpprofile,
		parse_hardware_profile_xml, 0) < 0)
    goto cleanup;

  if (copy_hardware_profile(profile, tmpprofile) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_hardware_profile_list(&tmpprofile);
  SAFE_FREE(data);
  SAFE_FREE(url);
  curl_free(safeid);

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
	xml_error("image", "Failed to parse XML", "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("image", "Failed to parse XML", "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;
      image_cur = cur->children;
      while (image_cur != NULL) {
	if (image_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)image_cur->name, "description"))
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
	oom_error();
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
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (images == NULL) {
    invalid_argument_error("Images argument cannot be NULL");
    return -1;
  }

  thislink = find_by_rel_in_link_list(&api->links, "images");
  if (thislink == NULL) {
    link_error("images");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    get_url_error("images", thislink->href);
    return -1;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  *images = NULL;
  if (parse_xml(data, "images", (void **)images, parse_image_xml, 1) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_image_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_image *image)
{
  char *url = NULL;
  char *data = NULL;
  char *safeid;
  struct deltacloud_image *tmpimage = NULL;
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (id == NULL) {
    invalid_argument_error("Image ID cannot be NULL");
    return -1;
  }
  if (image == NULL) {
    invalid_argument_error("Image argument cannot be NULL");
    return -1;
  }

  safeid = curl_escape(id, 0);
  if (safeid == NULL) {
    oom_error();
    return -1;
  }

  if (asprintf(&url, "%s/images/%s", api->url, safeid) < 0) {
    oom_error();
    goto cleanup;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    get_url_error("image", url);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml(data, "image", (void **)&tmpimage, parse_image_xml, 0) < 0)
    goto cleanup;

  if (copy_image(image, tmpimage) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_image_list(&tmpimage);
  SAFE_FREE(data);
  SAFE_FREE(url);
  curl_free(safeid);

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
	oom_error();
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
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (instance_states == NULL) {
    invalid_argument_error("Instance states argument cannot be NULL");
    return -1;
  }

  thislink = find_by_rel_in_link_list(&api->links, "instance_states");
  if (thislink == NULL) {
    link_error("instance_states");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    get_url_error("instance_states", thislink->href);
    return -1;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  *instance_states = NULL;
  if (parse_xml(data, "states", (void **)instance_states,
		parse_instance_state_xml, 1) < 0)
    goto cleanup;

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
  int ret = -1;

  /* despite the fact that 'name' is an input from the user, we don't
   * need to escape it since we are never using it as a URL
   */

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (name == NULL) {
    invalid_argument_error("Instance state name argument cannot be NULL");
    return -1;
  }
  if (instance_state == NULL) {
    invalid_argument_error("Instance state argument cannot be NULL");
    return -1;
  }

  instance_ret = deltacloud_get_instance_states(api, &statelist);
  if (instance_ret < 0) {
    dcloudprintf("Failed to get instance_states\n");
    return instance_ret;
  }

  found = find_by_name_in_instance_state_list(&statelist, name);
  if (found == NULL) {
    find_by_name_error(name, "instance_state");
    goto cleanup;
  }
  if (copy_instance_state(instance_state, found) < 0) {
    oom_error();
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
	STREQ((const char *)cur->name, "storage_volume")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("storage_volume", "Failed to parse XML",
		  "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("storage_volume", "Failed to parse XML",
		  "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;
      storage_cur = cur->children;
      while (storage_cur != NULL) {
	if (storage_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)storage_cur->name, "created"))
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
	oom_error();
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
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (storage_volumes == NULL) {
    invalid_argument_error("Storage volumes argument cannot be NULL");
    return -1;
  }

  thislink = find_by_rel_in_link_list(&api->links, "storage_volumes");
  if (thislink == NULL) {
    link_error("storage_volumes");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    get_url_error("storage_volumes", thislink->href);
    return -1;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  *storage_volumes = NULL;
  if (parse_xml(data, "storage_volumes", (void **)storage_volumes,
		parse_storage_volume_xml, 1) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_storage_volume_by_id(struct deltacloud_api *api,
					const char *id,
					struct deltacloud_storage_volume *storage_volume)
{
  char *url = NULL;
  char *data = NULL;
  char *safeid;
  struct deltacloud_storage_volume *tmpstorage_volume = NULL;
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (id == NULL) {
    invalid_argument_error("Storage volume ID cannot be NULL");
    return -1;
  }
  if (storage_volume == NULL) {
    invalid_argument_error("Storage volume argument cannot be NULL");
    return -1;
  }

  safeid = curl_escape(id, 0);
  if (safeid == NULL) {
    oom_error();
    return -1;
  }

  if (asprintf(&url, "%s/storage_volumes/%s", api->url, safeid) < 0) {
    oom_error();
    goto cleanup;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    get_url_error("storage_volume", url);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml(data, "storage_volume", (void **)&tmpstorage_volume,
		parse_storage_volume_xml, 0) < 0)
    goto cleanup;

  if (copy_storage_volume(storage_volume, tmpstorage_volume) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_storage_volume_list(&tmpstorage_volume);
  SAFE_FREE(data);
  SAFE_FREE(url);
  curl_free(safeid);

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
	STREQ((const char *)cur->name, "storage_snapshot")) {
      href = (char *)xmlGetProp(cur, BAD_CAST "href");
      if (href == NULL) {
	xml_error("storage_snapshot", "Failed to parse XML",
		  "did not see href property");
	goto cleanup;
      }

      id = (char *)xmlGetProp(cur, BAD_CAST "id");
      if (id == NULL) {
	xml_error("storage_snapshot", "Failed to parse XML",
		  "did not see id property");
	SAFE_FREE(href);
	goto cleanup;
      }

      ctxt->node = cur;
      snap_cur = cur->children;
      while (snap_cur != NULL) {
	if (snap_cur->type == XML_ELEMENT_NODE) {
	  if (STREQ((const char *)snap_cur->name, "created"))
	    created = getXPathString("string(./created)", ctxt);
	  else if (STREQ((const char *)snap_cur->name, "state"))
	    state = getXPathString("string(./state)", ctxt);
	  else if (STREQ((const char *)snap_cur->name, "storage_volume"))
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
	oom_error();
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
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (storage_snapshots == NULL) {
    invalid_argument_error("Storage snapshots argument cannot be NULL");
    return -1;
  }

  thislink = find_by_rel_in_link_list(&api->links, "storage_snapshots");
  if (thislink == NULL) {
    link_error("storage_snapshots");
    return -1;
  }

  data = get_url(thislink->href, api->user, api->password);
  if (data == NULL) {
    get_url_error("storage_snapshots", thislink->href);
    return -1;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  *storage_snapshots = NULL;
  if (parse_xml(data, "storage_snapshots", (void **)storage_snapshots,
		parse_storage_snapshot_xml, 1) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int deltacloud_get_storage_snapshot_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_storage_snapshot *storage_snapshot)
{
  char *url = NULL;
  char *data = NULL;
  char *safeid;
  struct deltacloud_storage_snapshot *tmpstorage_snapshot = NULL;
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (id == NULL) {
    invalid_argument_error("Storage snapshot ID cannot be NULL");
    return -1;
  }
  if (storage_snapshot == NULL) {
    invalid_argument_error("Storage snapshot argument cannot be NULL");
    return -1;
  }

  safeid = curl_escape(id, 0);
  if (safeid == NULL) {
    oom_error();
    return -1;
  }

  if (asprintf(&url, "%s/storage_snapshots/%s", api->url, safeid) < 0) {
    oom_error();
    goto cleanup;
  }

  data = get_url(url, api->user, api->password);
  if (data == NULL) {
    get_url_error("storage_snapshot", url);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml(data, "storage_snapshot", (void **)&tmpstorage_snapshot,
		parse_storage_snapshot_xml, 0) < 0)
    goto cleanup;

  if (copy_storage_snapshot(storage_snapshot, tmpstorage_snapshot) < 0) {
    oom_error();
    goto cleanup;
  }

  ret = 0;

 cleanup:
  deltacloud_free_storage_snapshot_list(&tmpstorage_snapshot);
  SAFE_FREE(data);
  SAFE_FREE(url);
  curl_free(safeid);

  return ret;
}

int deltacloud_create_instance(struct deltacloud_api *api, const char *image_id,
			       const char *name, const char *realm_id,
			       const char *hardware_profile,
			       struct deltacloud_instance *inst)
{
  struct deltacloud_link *thislink;
  size_t param_size;
  FILE *paramfp;
  int ret = -1;
  char *data = NULL;
  char *params = NULL;
  char *safename = NULL;
  char *saferealm = NULL;
  char *safehwp = NULL;
  int error = 0;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (image_id == NULL) {
    invalid_argument_error("Image ID cannot be NULL");
    return -1;
  }

  thislink = find_by_rel_in_link_list(&api->links, "instances");
  if (thislink == NULL) {
    link_error("instances");
    return -1;
  }

  paramfp = open_memstream(&params, &param_size);
  if (paramfp == NULL) {
    oom_error();
    return -1;
  }

  /* since image_id, name, realm_id, and hardware_profile come from the
   * user, we must not trust them and URL escape them before use
   */

  fprintf(paramfp, "image_id=%s", image_id);
  if (name != NULL) {
    safename = curl_escape(name, 0);
    if (safename == NULL)
      error = 1;
    fprintf(paramfp, "&name=%s", safename);
  }
  if (realm_id != NULL) {
    saferealm = curl_escape(realm_id, 0);
    if (saferealm == NULL)
      error = 1;
    fprintf(paramfp, "&realm_id=%s", saferealm);
  }
  if (hardware_profile != NULL) {
    safehwp = curl_escape(hardware_profile, 0);
    if (safehwp == NULL)
      error = 1;
    fprintf(paramfp, "&hwp_id=%s", safehwp);
  }
  fclose(paramfp);

  if (error) {
    /* FIXME: need a proper error here */
    dcloudprintf("Failed to escape input data\n");
    goto cleanup;
  }

  data = post_url(thislink->href, api->user, api->password, params, param_size);
  if (data == NULL) {
    post_url_error("create_instance", thislink->href);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_POST_URL_ERROR);
    goto cleanup;
  }

  if (inst != NULL) {
    memset(inst, 0, sizeof(struct deltacloud_instance));
    if (parse_one_instance(data, inst) < 0)
      goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(params);
  curl_free(safename);
  curl_free(saferealm);
  curl_free(safehwp);
  SAFE_FREE(data);

  return ret;
}

static int instance_action(struct deltacloud_api *api,
			   struct deltacloud_instance *instance,
			   const char *action_name)
{
  struct deltacloud_action *act;
  char *data;
  int ret = -1;

  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (instance == NULL) {
    invalid_argument_error("Instance cannot be NULL");
    return -1;
  }

  /* action_name can't possibly be NULL since it is not part of the
   * external API
   */

  act = find_by_rel_in_action_list(&instance->actions, action_name);
  if (act == NULL) {
    link_error(action_name);
    return -1;
  }

  data = post_url(act->href, api->user, api->password, NULL, 0);
  if (data == NULL) {
    post_url_error("action_name", act->href);
    return -1;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_POST_URL_ERROR);
    goto cleanup;
  }

  deltacloud_free_instance(instance);

  if (parse_one_instance(data, instance) < 0)
    goto cleanup;

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
  if (api == NULL) {
    invalid_argument_error("API cannot be NULL");
    return -1;
  }
  if (instance == NULL) {
    invalid_argument_error("Instance cannot be NULL");
    return -1;
  }

  /* in deltacloud the destroy action is a DELETE method, so we need
   * to use a different implementation
   */
  return delete_url(instance->href, api->user, api->password);
}

void deltacloud_free(struct deltacloud_api *api)
{
  if (api == NULL)
    return;

  free_link_list(&api->links);
  SAFE_FREE(api->user);
  SAFE_FREE(api->password);
  SAFE_FREE(api->url);
  memset(api, 0, sizeof(struct deltacloud_api));
}
