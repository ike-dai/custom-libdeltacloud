/*
 * Copyright (C) 2010,2011 Red Hat, Inc.
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
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include "libdeltacloud.h"
#include "common.h"
#include "curl_action.h"

/****************** ERROR REPORTING FUNCTIONS ********************************/
pthread_key_t deltacloud_last_error;

void invalid_argument_error(const char *details)
{
  set_error(DELTACLOUD_INVALID_ARGUMENT_ERROR, details);
}

static int parse_xml(const char *xml_string, const char *name,
		     int (*cb)(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			       void **data),
		     void **data);
static int parse_error_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data);
void set_xml_error(const char *xml, int type)
{
  char *errmsg = NULL;

  if (parse_xml(xml, "error", parse_error_xml, (void **)&errmsg) < 0)
    errmsg = strdup("Unknown error");

  set_error(type, errmsg);

  SAFE_FREE(errmsg);
}

void link_error(const char *name)
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

void data_error(const char *name)
{
  char *tmp;
  int alloc_fail = 0;

  if (asprintf(&tmp, "Expected %s data, received nothing", name) < 0) {
    tmp = "Expected data, received nothing";
    alloc_fail = 1;
  }

  set_error(DELTACLOUD_GET_URL_ERROR, tmp);
  if (!alloc_fail)
    SAFE_FREE(tmp);
}

void oom_error(void)
{
  set_error(DELTACLOUD_OOM_ERROR, "Failed to allocate memory");
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

void set_error(int errnum, const char *details)
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

void set_curl_error(int errcode, const char *header, CURLcode res)
{
  char *errstr;
  int alloc_fail = 0;

  if (asprintf(&errstr, "%s: %s", header, curl_easy_strerror(res)) < 0) {
    errstr = "Failed to set URL header";
    alloc_fail = 1;
  }

  set_error(errcode, errstr);

  if (!alloc_fail)
    SAFE_FREE(errstr);
}

/********************** IMPLEMENTATIONS OF COMMON FUNCTIONS *****************/
int internal_destroy(const char *href, const char *user, const char *password)
{
  char *data = NULL;
  int ret = -1;

  if (delete_url(href, user, password, &data) < 0)
    /* delete_url already set the error */
    return -1;

  if (data != NULL && is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_DELETE_URL_ERROR);
    goto cleanup;
  }

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  return ret;
}

static int add_safe_value(FILE *fp, const char *name, const char *value)
{
  char *safevalue;

  safevalue = curl_escape(value, 0);
  if (safevalue == NULL) {
    oom_error();
    return -1;
  }

  /* if we are not at the beginning of the stream, we need to append a & */
  if (ftell(fp) != 0)
    fprintf(fp, "&");

  fprintf(fp, "%s=%s", name, safevalue);

  SAFE_FREE(safevalue);

  return 0;
}

int internal_post(struct deltacloud_api *api, const char *href,
		  struct deltacloud_create_parameter *params,
		  int params_length, char **data, char **headers)
{
  size_t param_string_length;
  FILE *paramfp;
  int ret = -1;
  char *param_string = NULL;
  int i;

  *data = NULL;
  *headers = NULL;

  paramfp = open_memstream(&param_string, &param_string_length);
  if (paramfp == NULL) {
    oom_error();
    return -1;
  }

  /* since the parameters come from the user, we must not trust them and
   * URL escape them before use
   */

  for (i = 0; i < params_length; i++) {
    if (params[i].value != NULL) {
      if (add_safe_value(paramfp, params[i].name, params[i].value) < 0)
	/* add_safe_value already set the error */
	goto cleanup;
    }
  }

  fclose(paramfp);
  paramfp = NULL;

  if (post_url(href, api->user, api->password, param_string, data,
	       headers) != 0)
    /* post_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (*data != NULL && is_error_xml(*data)) {
    set_xml_error(*data, DELTACLOUD_POST_URL_ERROR);
    SAFE_FREE(*data);
    SAFE_FREE(*headers);
    goto cleanup;
  }

  ret = 0;

 cleanup:
  if (paramfp != NULL)
    fclose(paramfp);
  SAFE_FREE(param_string);

  return ret;
}

int internal_create(struct deltacloud_api *api, const char *link,
		    struct deltacloud_create_parameter *params,
		    int params_length, char **data, char **headers)
{
  struct deltacloud_link *thislink;

  thislink = api_find_link(api, link);
  if (thislink == NULL)
    /* api_find_link set the error */
    return -1;

  return internal_post(api, thislink->href, params, params_length, data,
		       headers);
}

/*
 * An internal function for fetching all of the elements of a particular
 * type.  Note that although relname and rootname is the same for almost
 * all types, there are a couple of them that don't conform to this pattern.
 */
int internal_get(struct deltacloud_api *api, const char *relname,
		 const char *rootname,
		 int (*xml_cb)(xmlNodePtr, xmlXPathContextPtr, void **),
		 void **output)
{
  struct deltacloud_link *thislink;
  char *data = NULL;
  int ret = -1;

  /* we only check api and output here, as those are the only parameters from
   * the user
   */
  if (!valid_api(api) || !valid_arg(output))
    return -1;

  thislink = api_find_link(api, relname);
  if (thislink == NULL)
    /* api_find_link set the error */
    return -1;

  if (get_url(thislink->href, api->user, api->password, &data) != 0)
    /* get_url sets its own errors, so don't overwrite it here */
    return -1;

  if (data == NULL) {
    /* if we made it here, it means that the transfer was successful (ret
     * was 0), but the data that we expected wasn't returned.  This is probably
     * a deltacloud server bug, so just set an error and bail out
     */
    data_error(relname);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  *output = NULL;
  if (parse_xml(data, rootname, xml_cb, output) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);

  return ret;
}

int internal_get_by_id(struct deltacloud_api *api, const char *id,
		       const char *relname, const char *rootname,
		       int (*cb)(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				 void *data),
		       void *output)
{
  char *url = NULL;
  char *data = NULL;
  char *safeid;
  int ret = -1;

  /* we only check api, id, and output here, as those are the only parameters
   * from the user
   */
  if (!valid_api(api) || !valid_arg(id) || !valid_arg(output))
    return -1;

  safeid = curl_escape(id, 0);
  if (safeid == NULL) {
    oom_error();
    return -1;
  }

  if (asprintf(&url, "%s/%s/%s", api->url, relname, safeid) < 0) {
    oom_error();
    goto cleanup;
  }

  if (get_url(url, api->user, api->password, &data) != 0)
    /* get_url sets its own errors, so don't overwrite it here */
    goto cleanup;

  if (data == NULL) {
    /* if we made it here, it means that the transfer was successful (ret
     * was 0), but the data that we expected wasn't returned.  This is probably
     * a deltacloud server bug, so just set an error and bail out
     */
    data_error(relname);
    goto cleanup;
  }

  if (is_error_xml(data)) {
    set_xml_error(data, DELTACLOUD_GET_URL_ERROR);
    goto cleanup;
  }

  if (parse_xml_single(data, rootname, cb, output) < 0)
    /* parse_xml_single set the error */
    goto cleanup;

  ret = 0;

 cleanup:
  SAFE_FREE(data);
  SAFE_FREE(url);
  curl_free(safeid);

  return ret;
}

/************************** XML PARSING FUNCTIONS ****************************/
static int parse_error_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void **data)
{
  char **msg = (char **)data;

  *msg = getXPathString("string(/error/message)", ctxt);

  if (*msg == NULL)
    *msg = strdup("Unknown error");
  else {
    /* the value we get back may have leading and trailing whitespace, so strip
     * it here
     */
    strip_trailing_whitespace(*msg);
    strip_leading_whitespace(*msg);
  }

  return 0;
}

int is_error_xml(const char *xml)
{
  return STRPREFIX(xml, "<error");
}

static void set_error_from_xml(const char *name, const char *usermsg)
{
  xmlErrorPtr last;
  char *msg;

  last = xmlGetLastError();
  if (last != NULL)
    msg = last->message;
  else
    msg = "unknown error";
  xml_error(name, usermsg, msg);
}

int internal_xml_parse(const char *xml_string, const char *name, xml_cb cb,
		       int single, void *output)
{
  xmlDocPtr xml;
  xmlNodePtr root;
  xmlXPathContextPtr ctxt = NULL;
  int ret = -1;
  int rc;

  xml = xmlReadDoc(BAD_CAST xml_string, name, NULL,
		   XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOERROR |
		   XML_PARSE_NOWARNING);
  if (!xml) {
    set_error_from_xml(name, "Failed to parse XML");
    return -1;
  }

  root = xmlDocGetRootElement(xml);
  if (root == NULL) {
    set_error_from_xml(name, "Failed to get the root element");
    goto cleanup;
  }

  if (STRNEQ((const char *)root->name, name)) {
    xml_error(name, "Failed to get expected root element", (char *)root->name);
    goto cleanup;
  }

  ctxt = xmlXPathNewContext(xml);
  if (ctxt == NULL) {
    set_error_from_xml(name, "Failed to initialize XPath context");
    goto cleanup;
  }

  ctxt->node = root;

  /* if "single" is true, then the XML looks something like:
   * <instance> ... </instance>
   * if "single" is false, then the XML looks something like:
   * <instances> <instance> ... </instance> </instances>"
   */
  if (single)
    rc = cb(root, ctxt, output);
  else
    rc = cb(root->children, ctxt, output);

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

int parse_xml_single(const char *xml_string, const char *name,
		     xml_cb cb,
		     void *output)
{
  return internal_xml_parse(xml_string, name, cb, 1, output);
}

char *getXPathString(const char *xpath, xmlXPathContextPtr ctxt)
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

static int parse_xml(const char *xml_string, const char *name,
		     int (*cb)(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			       void **data),
		     void **data)
{
  /* note the cast from the function pointer with void ** to xml_cb
   * (a function pointer with void *).  This is a little wonky, but safe
   * since we know sizeof(void *) == sizeof(void **)
   */
  return internal_xml_parse(xml_string, name, (xml_cb)cb, 0, data);
}

/************************ MISCELLANEOUS FUNCTIONS ***************************/
struct deltacloud_link *api_find_link(struct deltacloud_api *api,
				      const char *name)
{
  struct deltacloud_link *thislink;

  deltacloud_for_each(thislink, api->links) {
    if (STREQ(thislink->rel, name))
      break;
  }
  if (thislink == NULL)
    link_error(name);

  return thislink;
}

void free_parameters(struct deltacloud_create_parameter *params,
		     int params_length)
{
  int i;

  for (i = 0; i < params_length; i++)
    deltacloud_free_parameter_value(&params[i]);
}

int copy_parameters(struct deltacloud_create_parameter *dst,
		    struct deltacloud_create_parameter *src,
		    int params_length)
{
  int i;

  for (i = 0; i < params_length; i++) {
    if (deltacloud_prepare_parameter(&dst[i], src[i].name, src[i].value) < 0)
      /* this isn't entirely orthogonal, since this function can allocate
       * memory that it doesn't free.  We just depend on the higher layers
       * to free the whole thing as necessary
       */
      /* deltacloud_prepare_parameter already set the error */
      return -1;
  }

  return i;
}

void free_and_null(void *ptrptr)
{
  free (*(void**)ptrptr);
  *(void**)ptrptr = NULL;
}

void dcloudprintf(const char *fmt, ...)
{
  va_list va_args;

  if (getenv("LIBDELTACLOUD_DEBUG")) {
    va_start(va_args, fmt);
    vfprintf(stderr, fmt, va_args);
    va_end(va_args);
  }
}

void deltacloud_error_free_data(void *data)
{
  struct deltacloud_error *err = data;

  if (err == NULL)
    return;

  SAFE_FREE(err->details);
  SAFE_FREE(err);
}

int valid_api(struct deltacloud_api *api)
{
  if (!valid_arg(api))
    return 0;

  if (api->initialized != 0xfeedbeef) {
    invalid_argument_error("API is not properly initialized");
    return 0;
  }

  return 1;
}

void strip_trailing_whitespace(char *msg)
{
  int i;

  for (i = strlen(msg) - 1; i >= 0; i--) {
    if (msg[i] != ' ' && msg[i] != '\t' && msg[i] != '\n')
      break;

    msg[i] = '\0';
  }
}

void strip_leading_whitespace(char *msg)
{
  char *p;

  p = msg;
  while (*p == ' ' || *p == '\t' || *p == '\n')
    p++;

  /* use strlen(p) + 1 to make sure to copy the \0 */
  memmove(msg, p, strlen(p) + 1);
}

/****************************** DEBUG FUNCTIONS *****************************/
#ifdef DEBUG

#ifndef FAILRATE
#define FAILRATE 1000
#endif

static void seed_random(void)
{
  static int done = 0;

  if (done)
    return;

  srand(time(NULL));
  done = 1;
}

#undef xmlReadDoc
xmlDocPtr xmlReadDoc_sometimes_fail(const xmlChar *cur,
				    const char *URL,
				    const char *encoding,
				    int options)
{
  seed_random();

  if (rand() % FAILRATE)
    return xmlReadDoc(cur, URL, encoding, options);
  else
    return NULL;
}

#undef xmlDocGetRootElement
xmlNodePtr xmlDocGetRootElement_sometimes_fail(xmlDocPtr doc)
{
  seed_random();

  if (rand() % FAILRATE)
    return xmlDocGetRootElement(doc);
  else
    return NULL;
}

#undef xmlXPathNewContext
xmlXPathContextPtr xmlXPathNewContext_sometimes_fail(xmlDocPtr doc)
{
  seed_random();

  if (rand() % FAILRATE)
    return xmlXPathNewContext(doc);
  else
    return NULL;
}

#undef xmlXPathEval
xmlXPathObjectPtr xmlXPathEval_sometimes_fail(const xmlChar *str,
					      xmlXPathContextPtr ctx)
{
  seed_random();

  if (rand() % FAILRATE)
    return xmlXPathEval(str, ctx);
  else
    return NULL;
}

#undef asprintf
int asprintf_sometimes_fail(char **strp, const char *fmt, ...)
{
  va_list ap;
  int ret;

  seed_random();

  if (rand() % FAILRATE) {
    va_start(ap, fmt);
    ret = vasprintf(strp, fmt, ap);
    va_end(ap);
    return ret;
  }
  else
    return -1;
}

#undef malloc
void *malloc_sometimes_fail(size_t size)
{
  seed_random();

  if (rand() % FAILRATE)
    return malloc(size);
  else
    return NULL;
}

#undef realloc
void *realloc_sometimes_fail(void *ptr, size_t size)
{
  seed_random();

  if (rand() % FAILRATE)
    return realloc(ptr, size);
  else
    return NULL;
}

#undef calloc
void *calloc_sometimes_fail(size_t nmemb, size_t size)
{
  seed_random();

  if (rand() % FAILRATE)
    return calloc(nmemb, size);
  else
    return NULL;
}

#undef curl_easy_init
CURL *curl_easy_init_sometimes_fail(void)
{
  seed_random();

  if (rand() % FAILRATE)
    return curl_easy_init();
  else
    return NULL;
}

#undef curl_slist_append
struct curl_slist *curl_slist_append_sometimes_fail(struct curl_slist *list,
						    const char *string)
{
  seed_random();

  if (rand() % FAILRATE)
    return curl_slist_append(list, string);
  else
    return NULL;
}

#undef curl_easy_perform
CURLcode curl_easy_perform_sometimes_fail(CURL *handle)
{
  seed_random();

  if (rand() % FAILRATE)
    return curl_easy_perform(handle);
  else
    return CURLE_OUT_OF_MEMORY;
}

#undef strdup
char *strdup_sometimes_fail(const char *s)
{
  seed_random();

  if (rand() % FAILRATE)
    return strdup(s);
  else
    return NULL;
}

#endif /* DEBUG */
