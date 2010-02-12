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

void print_storage_volume_list(struct storage_volume **storage_volumes,
			       FILE *stream)
{
  struct storage_volume *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *storage_volumes;
  while (curr != NULL) {
    fprintf(stream, "Href: %s\n", curr->href);
    fprintf(stream, "ID: %s\n", curr->id);
    fprintf(stream, "Created: %s\n", curr->created);
    fprintf(stream, "State: %s\n", curr->state);
    fprintf(stream, "Capacity: %s\n", curr->capacity);
    fprintf(stream, "Device: %s\n", curr->device);
    fprintf(stream, "Instance Href: %s\n", curr->instance_href);
    curr = curr->next;
  }
}

void free_storage_volume_list(struct storage_volume **storage_volumes)
{
  struct storage_volume *curr, *next;

  curr = *storage_volumes;
  while (curr != NULL) {
    next = curr->next;
    free(curr->href);
    free(curr->id);
    free(curr->created);
    free(curr->state);
    free(curr->capacity);
    free(curr->device);
    free(curr->instance_href);
    free(curr);
    curr = next;
  }

  *storage_volumes = NULL;
}
