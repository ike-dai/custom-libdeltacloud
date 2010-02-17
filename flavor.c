#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "flavor.h"

void copy_flavor(struct flavor *dst, struct flavor *src)
{
  dst->href = strdup_or_null(src->href);
  dst->id = strdup_or_null(src->id);
  dst->memory = strdup_or_null(src->memory);
  dst->storage = strdup_or_null(src->storage);
  dst->architecture = strdup_or_null(src->architecture);
  dst->next = NULL;
}

int add_to_flavor_list(struct flavor **flavors, const char *href,
		       const char *id, const char *memory, const char *storage,
		       const char *architecture)
{
  struct flavor *oneflavor, *curr, *last;

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
    curr = *flavors;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneflavor;
  }

  return 0;
}

void print_flavor(struct flavor *flavor, FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "Href: %s\n", flavor->href);
  fprintf(stream, "ID: %s\n", flavor->id);
  fprintf(stream, "Memory: %s\n", flavor->memory);
  fprintf(stream, "Storage: %s\n", flavor->storage);
  fprintf(stream, "Architecture: %s\n", flavor->architecture);
}

void print_flavor_list(struct flavor **flavors, FILE *stream)
{
  struct flavor *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *flavors;
  while (curr != NULL) {
    print_flavor(curr, stream);
    curr = curr->next;
  }
}

void free_flavor(struct flavor *flavor)
{
  MY_FREE(flavor->href);
  MY_FREE(flavor->id);
  MY_FREE(flavor->memory);
  MY_FREE(flavor->storage);
  MY_FREE(flavor->architecture);
}

void free_flavor_list(struct flavor **flavors)
{
  struct flavor *curr, *next;

  curr = *flavors;
  while (curr != NULL) {
    next = curr->next;
    free_flavor(curr);
    MY_FREE(curr);
    curr = next;
  }

  *flavors = NULL;
}
