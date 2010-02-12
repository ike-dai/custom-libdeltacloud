#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "link.h"

int add_to_link_list(struct link **links, char *href, char *rel)
{
  struct link *onelink, *now, *last;

  onelink = malloc(sizeof(struct link));
  if (onelink == NULL)
    return -1;

  onelink->href = strdup_or_null(href);
  onelink->rel = strdup_or_null(rel);
  onelink->next = NULL;

  if (*links == NULL)
    /* First element in the list */
    *links = onelink;
  else {
    now = *links;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = onelink;
  }

  return 0;
}

void print_link_list(struct link **links, FILE *stream)
{
  struct link *now;

  if (stream == NULL)
    stream = stderr;

  now = *links;
  while (now != NULL) {
    fprintf(stream, "HREF %s, REL %s\n", now->href, now->rel);
    now = now->next;
  }
}

char *find_href_by_rel_in_link_list(struct link **links, char *rel)
{
  struct link *now;

  now = *links;
  while (now != NULL) {
    if (STREQ(now->rel, rel))
      return now->href;
    now = now->next;
  }

  return NULL;
}

void free_link_list(struct link **links)
{
  struct link *now, *next;

  now = *links;
  while (now != NULL) {
    next = now->next;
    free(now->href);
    free(now->rel);
    free(now);
    now = next;
  }

  *links = NULL;
}
