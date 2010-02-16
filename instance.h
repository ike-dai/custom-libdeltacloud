#ifndef INSTANCES_H
#define INSTANCES_H

struct action {
  char *rel;
  char *href;

  struct action *next;
};

struct address {
  char *address;

  struct address *next;
};

struct instance {
  char *id;
  char *name;
  char *owner_id;
  char *image_href;
  char *flavor_href;
  char *realm_href;
  char *state;
  struct action *actions;
  struct address *public_addresses;
  struct address *private_addresses;

  struct instance *next;
};

int add_to_address_list(struct address **addreses, char *address);
void print_address_list(struct address **addresses, FILE *stream);
void free_address_list(struct address **addresses);

int add_to_action_list(struct action **actions, char *rel, char *href);
void print_action_list(struct action **actions, FILE *stream);
void free_action_list(struct action **actions);

int add_to_instance_list(struct instance **instances, const char *id,
			 const char *name, const char *owner_id,
			 const char *image_href, const char *flavor_href,
			 const char *realm_href, const char *state,
			 struct action *actions,
			 struct address *public_addresses,
			 struct address *private_addresses);
void copy_instance(struct instance *dst, struct instance *src);
void print_instance(struct instance *instance, FILE *stream);
void print_instance_list(struct instance **instances, FILE *stream);
void free_instance(struct instance *instance);
void free_instance_list(struct instance **instances);

#endif
