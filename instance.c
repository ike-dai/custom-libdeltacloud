#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "instance.h"

int add_to_address_list(struct address **addresses, char *address)
{
  struct address *oneaddress, *curr, *last;

  oneaddress = malloc(sizeof(struct address));
  if (oneaddress == NULL)
    return -1;

  oneaddress->address = strdup_or_null(address);
  oneaddress->next = NULL;

  if (*addresses == NULL)
    /* First element in the list */
    *addresses = oneaddress;
  else {
    curr = *addresses;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneaddress;
  }

  return 0;
}

void print_address_list(struct address **addresses, FILE *stream)
{
  struct address *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *addresses;
  while (curr != NULL) {
    fprintf(stream, "Address: %s\n", curr->address);
    curr = curr->next;
  }
}

void free_address_list(struct address **addresses)
{
  struct address *curr, *next;

  curr = *addresses;
  while (curr != NULL) {
    next = curr->next;
    free(curr->address);
    free(curr);
    curr = next;
  }

  *addresses = NULL;
}

int add_to_action_list(struct action **actions, char *rel, char *href)
{
  struct action *oneaction, *curr, *last;

  oneaction = malloc(sizeof(struct action));
  if (oneaction == NULL)
    return -1;

  oneaction->rel = strdup_or_null(rel);
  oneaction->href = strdup_or_null(href);
  oneaction->next = NULL;

  if (*actions == NULL)
    /* First element in the list */
    *actions = oneaction;
  else {
    curr = *actions;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneaction;
  }

  return 0;
}

void print_action_list(struct action **actions, FILE *stream)
{
  struct action *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *actions;
  while (curr != NULL) {
    fprintf(stream, "Rel: %s\n", curr->rel);
    fprintf(stream, "Href: %s\n", curr->href);
    curr = curr->next;
  }
}

void free_action_list(struct action **actions)
{
  struct action *curr, *next;

  curr = *actions;
  while (curr != NULL) {
    next = curr->next;
    free(curr->rel);
    free(curr->href);
    free(curr);
    curr = next;
  }

  *actions = NULL;
}

int add_to_instance_list(struct instance **instances, const char *id,
			 const char *name, const char *owner_id,
			 const char *image_href, const char *flavor_href,
			 const char *realm_href, const char *state,
			 struct action *actions,
			 struct address *public_addresses,
			 struct address *private_addresses)
{
  struct instance *oneinstance, *curr, *last;

  oneinstance = malloc(sizeof(struct instance));
  if (oneinstance == NULL)
    return -1;

  oneinstance->id = strdup_or_null(id);
  oneinstance->name = strdup_or_null(name);
  oneinstance->owner_id = strdup_or_null(owner_id);
  oneinstance->image_href = strdup_or_null(image_href);
  oneinstance->flavor_href = strdup_or_null(flavor_href);
  oneinstance->realm_href = strdup_or_null(realm_href);
  oneinstance->state = strdup_or_null(state);
  oneinstance->actions = actions;
  oneinstance->public_addresses = public_addresses;
  oneinstance->private_addresses = private_addresses;
  oneinstance->next = NULL;

  if (*instances == NULL)
    /* First element in the list */
    *instances = oneinstance;
  else {
    curr = *instances;
    while (curr != NULL) {
      last = curr;
      curr = curr->next;
    }
    last->next = oneinstance;
  }

  return 0;
}

void print_instance_list(struct instance **instances, FILE *stream)
{
  struct instance *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *instances;
  while (curr != NULL) {
    fprintf(stream, "ID: %s\n", curr->id);
    fprintf(stream, "Name: %s\n", curr->name);
    fprintf(stream, "Owner ID: %s\n", curr->owner_id);
    fprintf(stream, "Image HREF: %s\n", curr->image_href);
    fprintf(stream, "Flavor HREF: %s\n", curr->flavor_href);
    fprintf(stream, "Realm HREF: %s\n", curr->realm_href);
    fprintf(stream, "State: %s\n", curr->state);
    print_action_list(&curr->actions, stream);
    print_address_list(&curr->public_addresses, stream);
    print_address_list(&curr->private_addresses, stream);
    curr = curr->next;
  }
}

void free_instance_list(struct instance **instances)
{
  struct instance *curr, *next;

  curr = *instances;
  while (curr != NULL) {
    next = curr->next;
    free(curr->id);
    free(curr->name);
    free(curr->owner_id);
    free(curr->image_href);
    free(curr->flavor_href);
    free(curr->realm_href);
    free(curr->state);
    free_action_list(&curr->actions);
    free_address_list(&curr->public_addresses);
    free_address_list(&curr->private_addresses);
    free(curr);
    curr = next;
  }

  *instances = NULL;
}
