#ifndef STORAGE_VOLUME_H
#define STORAGE_VOLUME_H

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
void copy_storage_volume(struct storage_volume *dst, struct storage_volume *src);
void print_storage_volume(struct storage_volume *storage_volume, FILE *stream);
void print_storage_volume_list(struct storage_volume **storage_volumes,
			       FILE *stream);
void free_storage_volume(struct storage_volume *storage_volume);
void free_storage_volume_list(struct storage_volume **storage_volumes);

#endif
