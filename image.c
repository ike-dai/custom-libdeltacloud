#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "image.h"

int add_to_image_list(struct image **images, const char *href, const char *id,
		      const char *description, const char *architecture,
		      const char *owner_id, const char *name)
{
  struct image *oneimage, *curr, *last;

  oneimage = malloc(sizeof(struct image));
  if (oneimage == NULL)
    return -1;

  oneimage->href = strdup_or_null(href);
  oneimage->id = strdup_or_null(id);
  oneimage->description = strdup_or_null(description);
  oneimage->architecture = strdup_or_null(architecture);
  oneimage->owner_id = strdup_or_null(owner_id);
  oneimage->name = strdup_or_null(name);
  oneimage->next = NULL;

  if (*images == NULL)
    /* First element in the list */
    *images = oneimage;
  else {
    curr = *images;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneimage;
  }

  return 0;
}

void print_image_list(struct image **images, FILE *stream)
{
  struct image *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *images;
  while (curr != NULL) {
    fprintf(stream, "Href: %s\n", curr->href);
    fprintf(stream, "ID: %s\n", curr->id);
    fprintf(stream, "Description: %s\n", curr->description);
    fprintf(stream, "Architecture: %s\n", curr->architecture);
    fprintf(stream, "Owner ID: %s\n", curr->owner_id);
    fprintf(stream, "Name: %s\n", curr->name);
    curr = curr->next;
  }
}

void free_image_list(struct image **images)
{
  struct image *curr, *next;

  curr = *images;
  while (curr != NULL) {
    next = curr->next;
    free(curr->href);
    free(curr->id);
    free(curr->description);
    free(curr->architecture);
    free(curr->owner_id);
    free(curr->name);
    free(curr);
    curr = next;
  }

  *images = NULL;
}
