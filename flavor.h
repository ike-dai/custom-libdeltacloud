#ifndef FLAVORS_H
#define FLAVORS_H

struct flavor {
  char *href;
  char *id;
  char *memory;
  char *storage;
  char *architecture;

  struct flavor *next;
};

int add_to_flavor_list(struct flavor **flavors, char *href, char *id,
		       char *memory, char *storage, char *architecture);
void print_flavors_list(struct flavor **flavors, FILE *stream);
void free_flavors_list(struct flavor **flavors);

#endif
