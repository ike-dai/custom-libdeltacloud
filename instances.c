#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "instances.h"

int add_to_address_list(struct address **addresses, char *address)
{
  struct address *oneaddress, *now, *last;

  oneaddress = malloc(sizeof(struct address));
  if (oneaddress == NULL)
    return -1;

  oneaddress->address = strdup_or_null(address);
  oneaddress->next = NULL;

  if (*addresses == NULL)
    /* First element in the list */
    *addresses = oneaddress;
  else {
    now = *addresses;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = oneaddress;
  }

  return 0;
}

void print_address_list(struct address **addresses, FILE *stream)
{
  struct address *now;

  if (stream == NULL)
    stream = stderr;

  now = *addresses;
  while (now != NULL) {
    fprintf(stream, "Address: %s\n", now->address);
    now = now->next;
  }
}

void free_address_list(struct address **addresses)
{
  struct address *now, *next;

  now = *addresses;
  while (now != NULL) {
    next = now->next;
    free(now->address);
    free(now);
    now = next;
  }

  *addresses = NULL;
}

int add_to_action_list(struct action **actions, char *rel, char *href)
{
  struct action *oneaction, *now, *last;

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
    now = *actions;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = oneaction;
  }

  return 0;
}

void print_action_list(struct action **actions, FILE *stream)
{
  struct action *now;

  if (stream == NULL)
    stream = stderr;

  now = *actions;
  while (now != NULL) {
    fprintf(stream, "Rel: %s\n", now->rel);
    fprintf(stream, "Href: %s\n", now->href);
    now = now->next;
  }
}

void free_action_list(struct action **actions)
{
  struct action *now, *next;

  now = *actions;
  while (now != NULL) {
    next = now->next;
    free(now->rel);
    free(now->href);
    free(now);
    now = next;
  }

  *actions = NULL;
}

int add_to_instance_list(struct instance **instances, const char *id,
			 const char *name, const char *owner_id,
			 const char *image_href, const char *flavor_href,
			 const char *realm_href, struct action *actions,
			 struct address *public_addresses,
			 struct address *private_addresses)
{
  struct instance *oneinstance, *now, *last;

  oneinstance = malloc(sizeof(struct instance));
  if (oneinstance == NULL)
    return -1;

  oneinstance->id = strdup_or_null(id);
  oneinstance->name = strdup_or_null(name);
  oneinstance->owner_id = strdup_or_null(owner_id);
  oneinstance->image_href = strdup_or_null(image_href);
  oneinstance->flavor_href = strdup_or_null(flavor_href);
  oneinstance->realm_href = strdup_or_null(realm_href);
  oneinstance->actions = actions;
  oneinstance->public_addresses = public_addresses;
  oneinstance->private_addresses = private_addresses;
  oneinstance->next = NULL;

  if (*instances == NULL)
    /* First element in the list */
    *instances = oneinstance;
  else {
    now = *instances;
    while (now != NULL) {
      last = now;
      now = now->next;
    }
    last->next = oneinstance;
  }

  return 0;
}

void print_instance_list(struct instance **instances, FILE *stream)
{
  struct instance *now;

  if (stream == NULL)
    stream = stderr;

  now = *instances;
  while (now != NULL) {
    fprintf(stream, "ID: %s\n", now->id);
    fprintf(stream, "Name: %s\n", now->name);
    fprintf(stream, "Owner ID: %s\n", now->owner_id);
    fprintf(stream, "Image HREF: %s\n", now->image_href);
    fprintf(stream, "Flavor HREF: %s\n", now->flavor_href);
    fprintf(stream, "Realm HREF: %s\n", now->realm_href);
    print_action_list(&now->actions, stream);
    print_address_list(&now->public_addresses, stream);
    print_address_list(&now->private_addresses, stream);
    now = now->next;
  }
}

void free_instance_list(struct instance **instances)
{
  struct instance *now, *next;

  now = *instances;
  while (now != NULL) {
    next = now->next;
    free(now->id);
    free(now->name);
    free(now->owner_id);
    free(now->image_href);
    free(now->flavor_href);
    free(now->realm_href);
    free_action_list(&now->actions);
    free_address_list(&now->public_addresses);
    free_address_list(&now->private_addresses);
    free(now);
    now = next;
  }

  *instances = NULL;
}
