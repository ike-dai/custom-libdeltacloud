#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"

void free_and_null(void *ptrptr)
{
  free (*(void**)ptrptr);
  *(void**)ptrptr = NULL;
}

char *strdup_or_null(const char *data)
{
  if (data != NULL)
    return strdup(data);

  return NULL;
}

#ifdef DEBUG

#undef xmlReadDoc
xmlDocPtr xmlReadDoc_sometimes_fail(const xmlChar *cur,
				    const char *URL,
				    const char *encoding,
				    int options)
{
  srand(time(NULL));

  if (rand() % 2)
    return xmlReadDoc(cur, URL, encoding, options);
  else
    return NULL;
}

#undef xmlDocGetRootElement
xmlNodePtr xmlDocGetRootElement_sometimes_fail(xmlDocPtr doc)
{
  srand(time(NULL));

  if (rand() % 2)
    return xmlDocGetRootElement(doc);
  else
    return NULL;
}

#undef xmlXPathNewContext
xmlXPathContextPtr xmlXPathNewContext_sometimes_fail(xmlDocPtr doc)
{
  srand(time(NULL));

  if (rand() % 2)
    return xmlXPathNewContext(doc);
  else
    return NULL;
}

#undef xmlXPathEval
xmlXPathObjectPtr xmlXPathEval_sometimes_fail(const xmlChar *str,
					      xmlXPathContextPtr ctx)
{
  srand(time(NULL));

  if (rand() % 2)
    return xmlXPathEval(str, ctx);
  else
    return NULL;
}

#undef asprintf
int asprintf_sometimes_fail(char **strp, const char *fmt, ...)
{
  va_list ap;
  int ret;

  srand(time(NULL));

  if (rand() % 2) {
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
  srand(time(NULL));

  if (rand() % 2)
    return malloc(size);
  else
    return NULL;
}

#endif
