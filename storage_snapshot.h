#ifndef STORAGE_SNAPSHOT_H
#define STORAGE_SNAPSHOT_H

struct storage_snapshot {
  char *href;
  char *id;
  char *created;
  char *state;
  char *storage_volume_href;

  struct storage_snapshot *next;
};

int add_to_storage_snapshot_list(struct storage_snapshot **storage_snapshots,
				 const char *href, const char *id,
				 const char *created, const char *state,
				 const char *storage_volume_href);
void print_storage_snapshot_list(struct storage_snapshot **storage_snapshots,
				 FILE *stream);
void free_storage_snapshot_list(struct storage_snapshot **storage_snapshots);

#endif
