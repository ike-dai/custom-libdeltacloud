#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "link.h"

int add_to_link_list(struct link **links, char *href, char *rel)
{
  struct link *onelink, *curr, *last;

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
    curr = *links;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = onelink;
  }

  return 0;
}

void print_link_list(struct link **links, FILE *stream)
{
  struct link *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *links;
  while (curr != NULL) {
    fprintf(stream, "HREF %s, REL %s\n", curr->href, curr->rel);
    curr = curr->next;
  }
}

char *find_href_by_rel_in_link_list(struct link **links, char *rel)
{
  struct link *curr;

  curr = *links;
  while (curr != NULL) {
    if (STREQ(curr->rel, rel))
      return curr->href;
    curr = curr->next;
  }

  return NULL;
}

void free_link_list(struct link **links)
{
  struct link *curr, *next;

  curr = *links;
  while (curr != NULL) {
    next = curr->next;
    free(curr->href);
    free(curr->rel);
    free(curr);
    curr = next;
  }

  *links = NULL;
}
