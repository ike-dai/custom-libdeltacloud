#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "realm.h"

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

void copy_realm(struct realm *dst, struct realm *src)
{
  dst->href = strdup_or_null(src->href);
  dst->id = strdup_or_null(src->id);
  dst->name = strdup_or_null(src->name);
  dst->state = strdup_or_null(src->state);
  dst->limit = strdup_or_null(src->limit);
  dst->next = NULL;
}

void print_realm(struct realm *realm, FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "Href: %s\n", realm->href);
  fprintf(stream, "ID: %s\n", realm->id);
  fprintf(stream, "Name: %s\n", realm->name);
  fprintf(stream, "Limit: %s\n", realm->limit);
  fprintf(stream, "State: %s\n", realm->state);
}

void print_realm_list(struct realm **realms, FILE *stream)
{
  struct realm *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *realms;
  while (curr != NULL) {
    print_realm(curr, stream);
    curr = curr->next;
  }
}

void free_realm(struct realm *realm)
{
  free(realm->href);
  free(realm->id);
  free(realm->name);
  free(realm->limit);
  free(realm->state);
}

void free_realm_list(struct realm **realms)
{
  struct realm *curr, *next;

  curr = *realms;
  while (curr != NULL) {
    next = curr->next;
    free_realm(curr);
    free(curr);
    curr = next;
  }

  *realms = NULL;
}
