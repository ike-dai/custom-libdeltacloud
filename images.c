#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "images.h"

int add_to_image_list(struct image **images, char *href, char *id,
		      char *description, char *architecture, char *owner_id)
{
  struct image *oneimage, *now, *last;

  oneimage = malloc(sizeof(struct image));
  if (oneimage == NULL)
    return -1;

  oneimage->href = strdup_or_null(href);
  oneimage->id = strdup_or_null(id);
  oneimage->description = strdup_or_null(description);
  oneimage->architecture = strdup_or_null(architecture);
  oneimage->owner_id = strdup_or_null(owner_id);
  oneimage->next = NULL;

  if (*images == NULL)
    /* First element in the list */
    *images = oneimage;
  else {
    now = *images;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = oneimage;
  }

  return 0;
}

void print_images_list(struct image **images, FILE *stream)
{
  struct image *now;

  if (stream == NULL)
    stream = stderr;

  now = *images;
  while (now != NULL) {
    fprintf(stream, "Href: %s\n", now->href);
    fprintf(stream, "ID: %s\n", now->id);
    fprintf(stream, "Description: %s\n", now->description);
    fprintf(stream, "Architecture: %s\n", now->architecture);
    fprintf(stream, "Owner ID: %s\n", now->owner_id);
    now = now->next;
  }
}

void free_images_list(struct image **images)
{
  struct image *now, *next;

  now = *images;
  while (now != NULL) {
    next = now->next;
    free(now->href);
    free(now->id);
    free(now->description);
    free(now->architecture);
    free(now->owner_id);
    free(now);
    now = next;
  }

  *images = NULL;
}
