#ifndef STORAGE_VOLUMES_H
#define STORAGE_VOLUMES_H

struct storage_volume {
  char *href;
  char *id;
  char *created;
  char *state;
  char *capacity;
  char *device;
  char *instance_href;

  struct storage_volume *next;
};

int add_to_storage_volume_list(struct storage_volume **storage_volumes,
			       const char *href, const char *id,
			       const char *created, const char *state,
			       const char *capacity, const char *device,
			       const char *instance_href);
void print_storage_volume_list(struct storage_volume **storage_volumes,
			       FILE *stream);
void free_storage_volume_list(struct storage_volume **storage_volumes);

#endif
