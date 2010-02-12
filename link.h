#ifndef LINK_H
#define LINK_H

struct link {
  char *href;
  char *rel;

  struct link *next;
};

int add_to_link_list(struct link **links, char *href, char *rel);
void print_link_list(struct link **links, FILE *stream);
struct link *find_by_rel_in_link_list(struct link **links, char *rel);
void free_link_list(struct link **links);

#endif
