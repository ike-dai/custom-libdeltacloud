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

  memset(onestorage_snapshot, 0, sizeof(struct storage_snapshot));

  if (strdup_or_null(&onestorage_snapshot->href, href) < 0)
    goto error;
  if (strdup_or_null(&onestorage_snapshot->id, id) < 0)
    goto error;
  if (strdup_or_null(&onestorage_snapshot->created, created) < 0)
    goto error;
  if (strdup_or_null(&onestorage_snapshot->state, state) < 0)
    goto error;
  if (strdup_or_null(&onestorage_snapshot->storage_volume_href, storage_volume_href) < 0)
    goto error;
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

 error:
  free_storage_snapshot(onestorage_snapshot);
  MY_FREE(onestorage_snapshot);
  return -1;
}

int copy_storage_snapshot(struct storage_snapshot *dst,
			  struct storage_snapshot *src)
{
  memset(dst, 0, sizeof(struct storage_snapshot));

  if (strdup_or_null(&dst->href, src->href) < 0)
    goto error;
  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->created, src->created) < 0)
    goto error;
  if (strdup_or_null(&dst->state, src->state) < 0)
    goto error;
  if (strdup_or_null(&dst->storage_volume_href, src->storage_volume_href) < 0)
    goto error;
  dst->next = NULL;

  return 0;

 error:
  free_storage_snapshot(dst);
  return -1;
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
  MY_FREE(storage_snapshot->href);
  MY_FREE(storage_snapshot->id);
  MY_FREE(storage_snapshot->created);
  MY_FREE(storage_snapshot->state);
  MY_FREE(storage_snapshot->storage_volume_href);
}

void free_storage_snapshot_list(struct storage_snapshot **storage_snapshots)
{
  struct storage_snapshot *curr, *next;

  curr = *storage_snapshots;
  while (curr != NULL) {
    next = curr->next;
    free_storage_snapshot(curr);
    MY_FREE(curr);
    curr = next;
  }

  *storage_snapshots = NULL;
}
