#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "storage_snapshot.h"

int add_to_storage_snapshot_list(struct storage_snapshot **storage_snapshots,
				 const char *href, const char *id,
				 const char *created, const char *state,
				 const char *storage_volume_href)
{
  struct storage_snapshot *onestorage_snapshot, *curr, *last;

  onestorage_snapshot = malloc(sizeof(struct storage_snapshot));
  if (onestorage_snapshot == NULL)
    return -1;

  onestorage_snapshot->href = strdup_or_null(href);
  onestorage_snapshot->id = strdup_or_null(id);
  onestorage_snapshot->created = strdup_or_null(created);
  onestorage_snapshot->state = strdup_or_null(state);
  onestorage_snapshot->storage_volume_href = strdup_or_null(storage_volume_href);
  onestorage_snapshot->next = NULL;

  if (*storage_snapshots == NULL)
    /* First element in the list */
    *storage_snapshots = onestorage_snapshot;
  else {
    curr = *storage_snapshots;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = onestorage_snapshot;
  }

  return 0;
}

void print_storage_snapshot_list(struct storage_snapshot **storage_snapshots,
				 FILE *stream)
{
  struct storage_snapshot *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *storage_snapshots;
  while (curr != NULL) {
    fprintf(stream, "Href: %s\n", curr->href);
    fprintf(stream, "ID: %s\n", curr->id);
    fprintf(stream, "Created: %s\n", curr->created);
    fprintf(stream, "State: %s\n", curr->state);
    fprintf(stream, "Storage Volume Href: %s\n", curr->storage_volume_href);
    curr = curr->next;
  }
}

void free_storage_snapshot_list(struct storage_snapshot **storage_snapshots)
{
  struct storage_snapshot *curr, *next;

  curr = *storage_snapshots;
  while (curr != NULL) {
    next = curr->next;
    free(curr->href);
    free(curr->id);
    free(curr->created);
    free(curr->state);
    free(curr->storage_volume_href);
    free(curr);
    curr = next;
  }

  *storage_snapshots = NULL;
}
