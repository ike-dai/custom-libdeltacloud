#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "storage_snapshots.h"

int add_to_storage_snapshot_list(struct storage_snapshot **storage_snapshots,
				 const char *href, const char *id,
				 const char *created, const char *state,
				 const char *storage_volume_href)
{
  struct storage_snapshot *onestorage_snapshot, *now, *last;

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
    now = *storage_snapshots;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = onestorage_snapshot;
  }

  return 0;
}

void print_storage_snapshot_list(struct storage_snapshot **storage_snapshots,
				 FILE *stream)
{
  struct storage_snapshot *now;

  if (stream == NULL)
    stream = stderr;

  now = *storage_snapshots;
  while (now != NULL) {
    fprintf(stream, "Href: %s\n", now->href);
    fprintf(stream, "ID: %s\n", now->id);
    fprintf(stream, "Created: %s\n", now->created);
    fprintf(stream, "State: %s\n", now->state);
    fprintf(stream, "Storage Volume Href: %s\n", now->storage_volume_href);
    now = now->next;
  }
}

void free_storage_snapshot_list(struct storage_snapshot **storage_snapshots)
{
  struct storage_snapshot *now, *next;

  now = *storage_snapshots;
  while (now != NULL) {
    next = now->next;
    free(now->href);
    free(now->id);
    free(now->created);
    free(now->state);
    free(now->storage_volume_href);
    free(now);
    now = next;
  }

  *storage_snapshots = NULL;
}
