#ifndef COMMON_H
#define COMMON_H

#define STREQ(a,b) (strcmp(a,b) == 0)
#define STRNEQ(a,b) (strcmp(a,b) != 0)

static inline char *strdup_or_null(const char *data)
{
  if (data != NULL)
    return strdup(data);

  return NULL;
}

#define MY_FREE(ptr) free_and_null(&(ptr))

static inline void free_and_null(void *ptrptr)
{
  free (*(void**)ptrptr);
  *(void**)ptrptr = NULL;
}

#endif
