#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "storage_volumes.h"

int add_to_storage_volume_list(struct storage_volume **storage_volumes,
			       const char *href, const char *id,
			       const char *created, const char *state,
			       const char *capacity, const char *device,
			       const char *instance_href)
{
  struct storage_volume *onestorage_volume, *now, *last;

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
    now = *storage_volumes;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = onestorage_volume;
  }

  return 0;
}

void print_storage_volume_list(struct storage_volume **storage_volumes,
			       FILE *stream)
{
  struct storage_volume *now;

  if (stream == NULL)
    stream = stderr;

  now = *storage_volumes;
  while (now != NULL) {
    fprintf(stream, "Href: %s\n", now->href);
    fprintf(stream, "ID: %s\n", now->id);
    fprintf(stream, "Created: %s\n", now->created);
    fprintf(stream, "State: %s\n", now->state);
    fprintf(stream, "Capacity: %s\n", now->capacity);
    fprintf(stream, "Device: %s\n", now->device);
    fprintf(stream, "Instance Href: %s\n", now->instance_href);
    now = now->next;
  }
}

void free_storage_volume_list(struct storage_volume **storage_volumes)
{
  struct storage_volume *now, *next;

  now = *storage_volumes;
  while (now != NULL) {
    next = now->next;
    free(now->href);
    free(now->id);
    free(now->created);
    free(now->state);
    free(now->capacity);
    free(now->device);
    free(now->instance_href);
    free(now);
    now = next;
  }

  *storage_volumes = NULL;
}
