#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "storage_volume.h"

int add_to_storage_volume_list(struct storage_volume **storage_volumes,
			       const char *href, const char *id,
			       const char *created, const char *state,
			       const char *capacity, const char *device,
			       const char *instance_href)
{
  struct storage_volume *onestorage_volume, *curr, *last;

  onestorage_volume = malloc(sizeof(struct storage_volume));
  if (onestorage_volume == NULL)
    return -1;

  onestorage_volume->href = strdup_or_null(href);
  onestorage_volume->id = strdup_or_null(id);
  onestorage_volume->created = strdup_or_null(created);
  onestorage_volume->state = strdup_or_null(state);
  onestorage_volume->capacity = strdup_or_null(capacity);
  onestorage_volume->device = strdup_or_null(device);
  onestorage_volume->instance_href = strdup_or_null(instance_href);
  onestorage_volume->next = NULL;

  if (*storage_volumes == NULL)
    /* First element in the list */
    *storage_volumes = onestorage_volume;
  else {
    curr = *storage_volumes;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = onestorage_volume;
  }

  return 0;
}

void copy_storage_volume(struct storage_volume *dst, struct storage_volume *src)
{
  dst->href = strdup_or_null(src->href);
  dst->id = strdup_or_null(src->id);
  dst->created = strdup_or_null(src->created);
  dst->state = strdup_or_null(src->state);
  dst->capacity = strdup_or_null(src->capacity);
  dst->device = strdup_or_null(src->device);
  dst->instance_href = strdup_or_null(src->instance_href);
  dst->next = NULL;
}

void print_storage_volume(struct storage_volume *storage_volume, FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "Href: %s\n", storage_volume->href);
  fprintf(stream, "ID: %s\n", storage_volume->id);
  fprintf(stream, "Created: %s\n", storage_volume->created);
  fprintf(stream, "State: %s\n", storage_volume->state);
  fprintf(stream, "Capacity: %s\n", storage_volume->capacity);
  fprintf(stream, "Device: %s\n", storage_volume->device);
  fprintf(stream, "Instance Href: %s\n", storage_volume->instance_href);
}

void print_storage_volume_list(struct storage_volume **storage_volumes,
			       FILE *stream)
{
  struct storage_volume *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *storage_volumes;
  while (curr != NULL) {
    print_storage_volume(curr, stream);
    curr = curr->next;
  }
}

void free_storage_volume(struct storage_volume *storage_volume)
{
  MY_FREE(storage_volume->href);
  MY_FREE(storage_volume->id);
  MY_FREE(storage_volume->created);
  MY_FREE(storage_volume->state);
  MY_FREE(storage_volume->capacity);
  MY_FREE(storage_volume->device);
  MY_FREE(storage_volume->instance_href);
}

void free_storage_volume_list(struct storage_volume **storage_volumes)
{
  struct storage_volume *curr, *next;

  curr = *storage_volumes;
  while (curr != NULL) {
    next = curr->next;
    free_storage_volume(curr);
    MY_FREE(curr);
    curr = next;
  }

  *storage_volumes = NULL;
}
