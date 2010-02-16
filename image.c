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

void copy_image(struct image *dst, struct image *src)
{
  dst->href = strdup_or_null(src->href);
  dst->id = strdup_or_null(src->id);
  dst->description = strdup_or_null(src->description);
  dst->architecture = strdup_or_null(src->architecture);
  dst->owner_id = strdup_or_null(src->owner_id);
  dst->name = strdup_or_null(src->name);
  dst->next = NULL;
}

void print_image(struct image *image, FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "Href: %s\n", image->href);
  fprintf(stream, "ID: %s\n", image->id);
  fprintf(stream, "Description: %s\n", image->description);
  fprintf(stream, "Architecture: %s\n", image->architecture);
  fprintf(stream, "Owner ID: %s\n", image->owner_id);
  fprintf(stream, "Name: %s\n", image->name);
}

void print_image_list(struct image **images, FILE *stream)
{
  struct image *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *images;
  while (curr != NULL) {
    print_image(curr, NULL);
    curr = curr->next;
  }
}

void free_image(struct image *image)
{
  free(image->href);
  free(image->id);
  free(image->description);
  free(image->architecture);
  free(image->owner_id);
  free(image->name);
}

void free_image_list(struct image **images)
{
  struct image *curr, *next;

  curr = *images;
  while (curr != NULL) {
    next = curr->next;
    free_image(curr);
    free(curr);
    curr = next;
  }

  *images = NULL;
}
