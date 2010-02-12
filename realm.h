#ifndef REALM_H
#define REALM_H

struct realm {
  char *href;
  char *id;
  char *name;
  char *limit;
  char *state;

  struct realm *next;
};

int add_to_realm_list(struct realm **realms, char *href, char *id, char *name,
		      char *state, char *limit);
void print_realm_list(struct realm **realms, FILE *stream);
void free_realm_list(struct realm **realms);

#endif
