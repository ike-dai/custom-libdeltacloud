#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "realms.h"

int add_to_realm_list(struct realm **realms, char *href, char *id, char *name,
		      char *state, char *limit)
{
  struct realm *onerealm, *now, *last;

  onerealm = malloc(sizeof(struct realm));
  if (onerealm == NULL)
    return -1;

  onerealm->href = strdup_or_null(href);
  onerealm->id = strdup_or_null(id);
  onerealm->name = strdup_or_null(name);
  onerealm->state = strdup_or_null(state);
  onerealm->limit = strdup_or_null(limit);
  onerealm->next = NULL;

  if (*realms == NULL)
    /* First element in the list */
    *realms = onerealm;
  else {
    now = *realms;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = onerealm;
  }

  return 0;
}

void print_realms_list(struct realm **realms, FILE *stream)
{
  struct realm *now;

  if (stream == NULL)
    stream = stderr;

  now = *realms;
  while (now != NULL) {
    fprintf(stream, "Href: %s\n", now->href);
    fprintf(stream, "ID: %s\n", now->id);
    fprintf(stream, "Name: %s\n", now->name);
    fprintf(stream, "Limit: %s\n", now->limit);
    fprintf(stream, "State: %s\n", now->state);
    now = now->next;
  }
}

void free_realms_list(struct realm **realms)
{
  struct realm *now, *next;

  now = *realms;
  while (now != NULL) {
    next = now->next;
    free(now->href);
    free(now->id);
    free(now->name);
    free(now->limit);
    free(now->state);
    free(now);
    now = next;
  }

  *realms = NULL;
}
