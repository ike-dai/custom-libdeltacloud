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

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libdeltacloud.h"
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <curl/curl.h>

/****************** ERROR REPORTING FUNCTIONS ********************************/
extern pthread_key_t deltacloud_last_error;
void invalid_argument_error(const char *details);
void set_xml_error(const char *xml, int type);
void link_error(const char *name);
void data_error(const char *name);
void oom_error(void);
void set_error(int errnum, const char *details);
void set_curl_error(int errcode, const char *header, CURLcode res);

/********************** IMPLEMENTATIONS OF COMMON FUNCTIONS *****************/
int internal_destroy(const char *href, const char *user, const char *password);
int internal_post(struct deltacloud_api *api, const char *href,
		  struct deltacloud_create_parameter *params,
		  int params_length, char **data, char **headers);
int internal_create(struct deltacloud_api *api, const char *link,
		    struct deltacloud_create_parameter *params,
		    int params_length, char **data, char **headers);
int internal_get(struct deltacloud_api *api, const char *relname,
		 const char *rootname,
		 int (*xml_cb)(xmlNodePtr, xmlXPathContextPtr, void **),
		 void **output);
int internal_get_by_id(struct deltacloud_api *api, const char *id,
		       const char *relname, const char *rootname,
		       int (*cb)(xmlNodePtr cur, xmlXPathContextPtr ctxt,
				 void *data),
		       void *output);

/************************** XML PARSING FUNCTIONS ****************************/
int is_error_xml(const char *xml);
typedef int (*xml_cb)(xmlNodePtr cur, xmlXPathContextPtr ctxt, void *data);
int internal_xml_parse(const char *xml_string, const char *name, xml_cb cb,
		       int single, void *output);
int parse_xml_single(const char *xml_string, const char *name,
		     int (*cb)(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			       void *data),
		     void *output);
char *getXPathString(const char *xpath, xmlXPathContextPtr ctxt);

/************************ MISCELLANEOUS FUNCTIONS ***************************/
struct deltacloud_link *api_find_link(struct deltacloud_api *api,
				      const char *name);
void free_parameters(struct deltacloud_create_parameter *params,
		     int params_length);
int copy_parameters(struct deltacloud_create_parameter *dst,
		    struct deltacloud_create_parameter *src,
		    int params_length);
void free_and_null(void *ptrptr);
void dcloudprintf(const char *fmt, ...);
void deltacloud_error_free_data(void *data);
int valid_api(struct deltacloud_api *api);
void strip_trailing_whitespace(char *msg);
void strip_leading_whitespace(char *msg);
#define valid_arg(x) ((x == NULL) ? invalid_argument_error(#x " cannot be NULL"), 0 : 1)
#define STREQ(a,b) (strcmp(a,b) == 0)
#define STRNEQ(a,b) (strcmp(a,b) != 0)
#define STRPREFIX(a,b) (strncmp(a,b,strlen(b)) == 0)
#define SAFE_FREE(ptr) free_and_null(&(ptr))

#define add_to_list(list, type, element) do { \
    type *curr, *last;			      \
    if (*list == NULL)			      \
      /* First element in the list */	      \
      *list = element;			      \
    else {				      \
      curr = *list;			      \
      while (curr != NULL) {		      \
	last = curr;			      \
	curr = curr->next;		      \
      }					      \
      last->next = element;		      \
    }					      \
  } while(0)

#define free_list(list, type, cb) do {		\
    type *curr, *next;				\
    if (list == NULL)				\
      return;					\
    curr = *list;				\
    while (curr != NULL) {			\
      next = curr->next;			\
      cb(curr);					\
      SAFE_FREE(curr);				\
      curr = next;				\
    }						\
    *list = NULL;				\
  } while(0)


#ifdef __cplusplus
}
#endif

/****************************** DEBUG FUNCTIONS *****************************/
#ifdef DEBUG
#include <libxml/parser.h>
#include <libxml/xpath.h>
xmlDocPtr xmlReadDoc_sometimes_fail(const xmlChar *cur,
				    const char *URL,
				    const char *encoding,
				    int options);
#define xmlReadDoc xmlReadDoc_sometimes_fail

xmlNodePtr xmlDocGetRootElement_sometimes_fail(xmlDocPtr doc);
#define xmlDocGetRootElement xmlDocGetRootElement_sometimes_fail

#define xmlXPathNewContextr xmlXPathNewContext_sometimes_fail
xmlXPathContextPtr xmlXPathNewContext_sometimes_fail(xmlDocPtr doc);

#define xmlXPathEval xmlXPathEval_sometimes_fail
xmlXPathObjectPtr xmlXPathEval_sometimes_fail(const xmlChar *str,
					      xmlXPathContextPtr ctx);

#define asprintf asprintf_sometimes_fail
int asprintf_sometimes_fail(char **strp, const char *fmt, ...);

#define malloc malloc_sometimes_fail
void *malloc_sometimes_fail(size_t size);

#define realloc realloc_sometimes_fail
void *realloc_sometimes_fail(void *ptr, size_t size);

#define calloc calloc_sometimes_fail
void *calloc_sometimes_fail(size_t nmemb, size_t size);

/* some versions of gcc #define strdup to an internal function.  However,
 * we can work around this by undefining it ourselves, and getting a possibly
 * slower version.  Since this is for memory-leak debugging purposes, that
 * suits our use-case fine
 */
#ifdef strdup
#undef strdup
#endif
#define strdup strdup_sometimes_fail
char *strdup_sometimes_fail(const char *s);

#include <curl/curl.h>
#define curl_easy_init curl_easy_init_sometimes_fail
CURL *curl_easy_init_sometimes_fail(void);

#define curl_slist_append curl_slist_append_sometimes_fail
struct curl_slist *curl_slist_append_sometimes_fail(struct curl_slist *list,
						    const char *string);

/* It would be nice to redefine curl_easy_setopt() to sometimes fail as well.
 * Unfortunately it has a prototype of:
 *
 * CURL_EXTERN CURLcode curl_easy_setopt(CURL *curl, CURLoption option, ...);
 *
 * Because of the ellipses, and because of the lack of a va* variant, we can't
 * pass the variable argument list through.  Another attempt I made was to
 * define curl_easy_setopt_sometimes_fail(CURL *curl, CURLoption option, void *ptr),
 * but this failed because of casting between void * and the real type.  Yet
 * another attempt was made by making the whole thing a macro (so we could
 * indeed pass the variable arguments through), but that failed because you
 * cannot return a value from a macro.
 */

#define curl_easy_perform curl_easy_perform_sometimes_fail
CURLcode curl_easy_perform_sometimes_fail(CURL *handle);

#endif

#endif
