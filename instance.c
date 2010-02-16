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

static void copy_address_list(struct address **dst, struct address **src)
{
  struct address *curr;

  *dst = NULL;

  curr = *src;
  while (curr != NULL) {
    add_to_address_list(dst, curr->address);
    curr = curr->next;
  }
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

static void copy_action_list(struct action **dst, struct action **src)
{
  struct action *curr;

  *dst = NULL;

  curr = *src;
  while (curr != NULL) {
    add_to_action_list(dst, curr->rel, curr->href);
    curr = curr->next;
  }
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

void copy_instance(struct instance *dst, struct instance *src)
{
  dst->id = strdup_or_null(src->id);
  dst->name = strdup_or_null(src->name);
  dst->owner_id = strdup_or_null(src->owner_id);
  dst->image_href = strdup_or_null(src->image_href);
  dst->flavor_href = strdup_or_null(src->flavor_href);
  dst->realm_href = strdup_or_null(src->realm_href);
  dst->state = strdup_or_null(src->state);
  copy_action_list(&dst->actions, &src->actions);
  copy_address_list(&dst->public_addresses, &src->public_addresses);
  copy_address_list(&dst->private_addresses, &src->private_addresses);
  dst->next = NULL;
}

void print_instance(struct instance *instance, FILE *stream)
{
  if (stream == NULL)
    stream = stderr;

  fprintf(stream, "ID: %s\n", instance->id);
  fprintf(stream, "Name: %s\n", instance->name);
  fprintf(stream, "Owner ID: %s\n", instance->owner_id);
  fprintf(stream, "Image HREF: %s\n", instance->image_href);
  fprintf(stream, "Flavor HREF: %s\n", instance->flavor_href);
  fprintf(stream, "Realm HREF: %s\n", instance->realm_href);
  fprintf(stream, "State: %s\n", instance->state);
  print_action_list(&instance->actions, stream);
  print_address_list(&instance->public_addresses, stream);
  print_address_list(&instance->private_addresses, stream);
}

void print_instance_list(struct instance **instances, FILE *stream)
{
  struct instance *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *instances;
  while (curr != NULL) {
    print_instance(curr, NULL);
    curr = curr->next;
  }
}

void free_instance(struct instance *instance)
{
  free(instance->id);
  free(instance->name);
  free(instance->owner_id);
  free(instance->image_href);
  free(instance->flavor_href);
  free(instance->realm_href);
  free(instance->state);
  free_action_list(&instance->actions);
  free_address_list(&instance->public_addresses);
  free_address_list(&instance->private_addresses);
}

void free_instance_list(struct instance **instances)
{
  struct instance *curr, *next;

  curr = *instances;
  while (curr != NULL) {
    next = curr->next;
    free_instance(curr);
    free(curr);
    curr = next;
  }

  *instances = NULL;
}
