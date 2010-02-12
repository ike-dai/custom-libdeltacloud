#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "flavor.h"

int add_to_flavor_list(struct flavor **flavors, char *href, char *id,
		       char *memory, char *storage, char *architecture)
{
  struct flavor *oneflavor, *now, *last;

  oneflavor = malloc(sizeof(struct flavor));
  if (oneflavor == NULL)
    return -1;

  oneflavor->href = strdup_or_null(href);
  oneflavor->id = strdup_or_null(id);
  oneflavor->memory = strdup_or_null(memory);
  oneflavor->storage = strdup_or_null(storage);
  oneflavor->architecture = strdup_or_null(architecture);
  oneflavor->next = NULL;

  if (*flavors == NULL)
    /* First element in the list */
    *flavors = oneflavor;
  else {
    now = *flavors;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = oneflavor;
  }

  return 0;
}

void print_flavor_list(struct flavor **flavors, FILE *stream)
{
  struct flavor *now;

  if (stream == NULL)
    stream = stderr;

  now = *flavors;
  while (now != NULL) {
    fprintf(stream, "Href: %s\n", now->href);
    fprintf(stream, "ID: %s\n", now->id);
    fprintf(stream, "Memory: %s\n", now->memory);
    fprintf(stream, "Storage: %s\n", now->storage);
    fprintf(stream, "Architecture: %s\n", now->architecture);
    now = now->next;
  }
}

void free_flavor_list(struct flavor **flavors)
{
  struct flavor *now, *next;

  now = *flavors;
  while (now != NULL) {
    next = now->next;
    free(now->href);
    free(now->id);
    free(now->memory);
    free(now->storage);
    free(now->architecture);
    free(now);
    now = next;
  }

  *flavors = NULL;
}
