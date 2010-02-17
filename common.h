#ifndef COMMON_H
#define COMMON_H

#define STREQ(a,b) (strcmp(a,b) == 0)
#define STRNEQ(a,b) (strcmp(a,b) != 0)

char *strdup_or_null(const char *data);

#define MY_FREE(ptr) free_and_null(&(ptr))
void free_and_null(void *ptrptr);

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
