#ifndef REALMS_H
#define REALMS_H

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
void print_realms_list(struct realm **realms, FILE *stream);
void free_realms_list(struct realm **realms);

#endif
