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

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define STREQ(a,b) (strcmp(a,b) == 0)
#define STRNEQ(a,b) (strcmp(a,b) != 0)
#define STRPREFIX(a,b) (strncmp(a,b,strlen(b)) == 0)

int strdup_or_null(char **out, const char *in);

#define SAFE_FREE(ptr) free_and_null(&(ptr))
void free_and_null(void *ptrptr);

void dcloudprintf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

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

#define strdup strdup_sometimes_fail
char *strdup_sometimes_fail(const char *s);

#include <curl/curl.h>
#define curl_easy_init curl_easy_init_sometimes_fail
CURL *curl_easy_init_sometimes_fail(void);

#define curl_slist_append curl_slist_append_sometimes_fail
struct curl_slist *curl_slist_append_sometimes_fail(struct curl_slist *list,
						    const char *string);

#define curl_easy_perform curl_easy_perform_sometimes_fail
CURLcode curl_easy_perform_sometimes_fail(CURL *handle);

#endif

#endif
