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

void copy_storage_snapshot(struct storage_snapshot *dst,
			   struct storage_snapshot *src)
{
  dst->href = strdup_or_null(src->href);
  dst->id = strdup_or_null(src->id);
  dst->created = strdup_or_null(src->created);
  dst->state = strdup_or_null(src->state);
  dst->storage_volume_href = strdup_or_null(src->storage_volume_href);
  dst->next = NULL;
}

void print_storage_snapshot(struct storage_snapshot *storage_snapshot,
			    FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "Href: %s\n", storage_snapshot->href);
  fprintf(stream, "ID: %s\n", storage_snapshot->id);
  fprintf(stream, "Created: %s\n", storage_snapshot->created);
  fprintf(stream, "State: %s\n", storage_snapshot->state);
  fprintf(stream, "Storage Volume Href: %s\n",
	  storage_snapshot->storage_volume_href);
}

void print_storage_snapshot_list(struct storage_snapshot **storage_snapshots,
				 FILE *stream)
{
  struct storage_snapshot *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *storage_snapshots;
  while (curr != NULL) {
    print_storage_snapshot(curr, NULL);
    curr = curr->next;
  }
}

void free_storage_snapshot(struct storage_snapshot *storage_snapshot)
{
  free(storage_snapshot->href);
  free(storage_snapshot->id);
  free(storage_snapshot->created);
  free(storage_snapshot->state);
  free(storage_snapshot->storage_volume_href);
}

void free_storage_snapshot_list(struct storage_snapshot **storage_snapshots)
{
  struct storage_snapshot *curr, *next;

  curr = *storage_snapshots;
  while (curr != NULL) {
    next = curr->next;
    free_storage_snapshot(curr);
    free(curr);
    curr = next;
  }

  *storage_snapshots = NULL;
}
