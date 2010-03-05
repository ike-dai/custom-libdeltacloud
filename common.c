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
#include <time.h>
#include <stdarg.h>
#include "common.h"

void free_and_null(void *ptrptr)
{
  free (*(void**)ptrptr);
  *(void**)ptrptr = NULL;
}

int strdup_or_null(char **out, const char *in)
{
  if (in == NULL) {
    *out = NULL;
    return 0;
  }

  *out = strdup(in);

  if (*out == NULL)
    return -1;

  return 0;
}

void dcloudprintf(const char *fmt, ...)
{
  va_list va_args;

  if (getenv("DCLOUDAPI_DEBUG")) {
    va_start(va_args, fmt);
    vfprintf(stderr, fmt, va_args);
    va_end(va_args);
  }
}

#ifdef DEBUG

#define FAILRATE 1000
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
    return -1;
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
