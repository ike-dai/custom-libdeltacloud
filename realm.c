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

  memset(onerealm, 0, sizeof(struct realm));

  if (strdup_or_null(&onerealm->href, href) < 0)
    goto error;
  if (strdup_or_null(&onerealm->id, id) < 0)
    goto error;
  if (strdup_or_null(&onerealm->name, name) < 0)
    goto error;
  if (strdup_or_null(&onerealm->state, state) < 0)
    goto error;
  if (strdup_or_null(&onerealm->limit, limit) < 0)
    goto error;
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

 error:
  free_realm(onerealm);
  MY_FREE(onerealm);
  return -1;
}

int copy_realm(struct realm *dst, struct realm *src)
{
  memset(dst, 0, sizeof(struct realm));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->name, src->name) < 0)
    goto error;
  if (strdup_or_null(&dst->state, src->state) < 0)
    goto error;
  if (strdup_or_null(&dst->limit, src->limit) < 0)
    goto error;
  dst->next = NULL;

  return 0;

 error:
  free_realm(dst);
  return -1;
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
  MY_FREE(realm->href);
  MY_FREE(realm->id);
  MY_FREE(realm->name);
  MY_FREE(realm->limit);
  MY_FREE(realm->state);
}

void free_realm_list(struct realm **realms)
{
  struct realm *curr, *next;

  curr = *realms;
  while (curr != NULL) {
    next = curr->next;
    free_realm(curr);
    MY_FREE(curr);
    curr = next;
  }

  *realms = NULL;
}
