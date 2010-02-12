#ifndef FLAVOR_H
#define FLAVOR_H

struct flavor {
  char *href;
  char *id;
  char *memory;
  char *storage;
  char *architecture;

  struct flavor *next;
};

int add_to_flavor_list(struct flavor **flavors, const char *href,
		       const char *id, const char *memory, const char *storage,
		       const char *architecture);
void print_flavor_list(struct flavor **flavors, FILE *stream);
void free_flavor_list(struct flavor **flavors);

#endif
