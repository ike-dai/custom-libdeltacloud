/*
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Chris Lalancette <clalance@redhat.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "instance.h"

static void free_address(struct deltacloud_address *addr)
{
  MY_FREE(addr->address);
}

int add_to_address_list(struct deltacloud_address **addresses, const char *address)
{
  struct deltacloud_address *oneaddress, *curr, *last;

  oneaddress = malloc(sizeof(struct deltacloud_address));
  if (oneaddress == NULL)
    return -1;

  memset(oneaddress, 0, sizeof(struct deltacloud_address));

  if (strdup_or_null(&oneaddress->address, address) < 0)
    goto error;
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

 error:
  free_address(oneaddress);
  MY_FREE(oneaddress);
  return -1;
}

static int copy_address_list(struct deltacloud_address **dst, struct deltacloud_address **src)
{
  struct deltacloud_address *curr;

  *dst = NULL;

  curr = *src;
  while (curr != NULL) {
    if (add_to_address_list(dst, curr->address) < 0)
      goto error;
    curr = curr->next;
  }

  return 0;

 error:
  free_address_list(dst);
  return -1;
}

void print_address_list(struct deltacloud_address **addresses, FILE *stream)
{
  struct deltacloud_address *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *addresses;
  while (curr != NULL) {
    fprintf(stream, "Address: %s\n", curr->address);
    curr = curr->next;
  }
}

void free_address_list(struct deltacloud_address **addresses)
{
  struct deltacloud_address *curr, *next;

  curr = *addresses;
  while (curr != NULL) {
    next = curr->next;
    free_address(curr);
    MY_FREE(curr);
    curr = next;
  }

  *addresses = NULL;
}

static void free_action(struct deltacloud_action *action)
{
  MY_FREE(action->rel);
  MY_FREE(action->href);
}

int add_to_action_list(struct deltacloud_action **actions, const char *rel,
		       const char *href)
{
  struct deltacloud_action *oneaction, *curr, *last;

  oneaction = malloc(sizeof(struct deltacloud_action));
  if (oneaction == NULL)
    return -1;

  memset(oneaction, 0, sizeof(struct deltacloud_action));

  if (strdup_or_null(&oneaction->rel, rel) < 0)
    goto error;
  if (strdup_or_null(&oneaction->href, href) < 0)
    goto error;
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

 error:
  free_action(oneaction);
  MY_FREE(oneaction);
  return -1;
}

struct deltacloud_action *find_by_rel_in_action_list(struct deltacloud_action **actions,
					  const char *rel)
{
  struct deltacloud_action *curr;

  curr = *actions;
  while (curr != NULL) {
    if (STREQ(curr->rel, rel))
      return curr;
    curr = curr->next;
  }

  return NULL;
}

static int copy_action_list(struct deltacloud_action **dst, struct deltacloud_action **src)
{
  struct deltacloud_action *curr;

  *dst = NULL;

  curr = *src;
  while (curr != NULL) {
    if (add_to_action_list(dst, curr->rel, curr->href) < 0)
      goto error;
    curr = curr->next;
  }

  return 0;

 error:
  free_action_list(dst);
  return -1;
}

void print_action_list(struct deltacloud_action **actions, FILE *stream)
{
  struct deltacloud_action *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *actions;
  while (curr != NULL) {
    fprintf(stream, "Rel: %s\n", curr->rel);
    fprintf(stream, "Href: %s\n", curr->href);
    curr = curr->next;
  }
}

void free_action_list(struct deltacloud_action **actions)
{
  struct deltacloud_action *curr, *next;

  curr = *actions;
  while (curr != NULL) {
    next = curr->next;
    free_action(curr);
    MY_FREE(curr);
    curr = next;
  }

  *actions = NULL;
}

int add_to_instance_list(struct deltacloud_instance **instances, const char *id,
			 const char *name, const char *owner_id,
			 const char *image_href, const char *flavor_href,
			 const char *realm_href, const char *state,
			 struct deltacloud_action *actions,
			 struct deltacloud_address *public_addresses,
			 struct deltacloud_address *private_addresses)
{
  struct deltacloud_instance *oneinstance, *curr, *last;

  oneinstance = malloc(sizeof(struct deltacloud_instance));
  if (oneinstance == NULL)
    return -1;

  memset(oneinstance, 0, sizeof(struct deltacloud_instance));

  if (strdup_or_null(&oneinstance->id, id) < 0)
    goto error;
  if (strdup_or_null(&oneinstance->name, name) < 0)
    goto error;
  if (strdup_or_null(&oneinstance->owner_id, owner_id) < 0)
    goto error;
  if (strdup_or_null(&oneinstance->image_href, image_href) < 0)
    goto error;
  if (strdup_or_null(&oneinstance->flavor_href, flavor_href) < 0)
    goto error;
  if (strdup_or_null(&oneinstance->realm_href, realm_href) < 0)
    goto error;
  if (strdup_or_null(&oneinstance->state, state) < 0)
    goto error;
  if (copy_action_list(&oneinstance->actions, &actions) < 0)
    goto error;
  if (copy_address_list(&oneinstance->public_addresses, &public_addresses) < 0)
    goto error;
  if (copy_address_list(&oneinstance->private_addresses, &private_addresses) < 0)
    goto error;
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

 error:
  free_instance(oneinstance);
  MY_FREE(oneinstance);
  return -1;
}

int copy_instance(struct deltacloud_instance *dst, struct deltacloud_instance *src)
{
  memset(dst, 0, sizeof(struct deltacloud_instance));

  if (strdup_or_null(&dst->id, src->id) < 0)
    goto error;
  if (strdup_or_null(&dst->name, src->name) < 0)
    goto error;
  if (strdup_or_null(&dst->owner_id, src->owner_id) < 0)
    goto error;
  if (strdup_or_null(&dst->image_href, src->image_href) < 0)
    goto error;
  if (strdup_or_null(&dst->flavor_href, src->flavor_href) < 0)
    goto error;
  if (strdup_or_null(&dst->realm_href, src->realm_href) < 0)
    goto error;
  if (strdup_or_null(&dst->state, src->state) < 0)
    goto error;
  copy_action_list(&dst->actions, &src->actions);
  copy_address_list(&dst->public_addresses, &src->public_addresses);
  copy_address_list(&dst->private_addresses, &src->private_addresses);
  dst->next = NULL;

  return 0;

 error:
  free_instance(dst);
  return -1;
}

void print_instance(struct deltacloud_instance *instance, FILE *stream)
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

void print_instance_list(struct deltacloud_instance **instances, FILE *stream)
{
  struct deltacloud_instance *curr;

  if (stream == NULL)
    stream = stderr;

  curr = *instances;
  while (curr != NULL) {
    print_instance(curr, NULL);
    curr = curr->next;
  }
}

void free_instance(struct deltacloud_instance *instance)
{
  MY_FREE(instance->id);
  MY_FREE(instance->name);
  MY_FREE(instance->owner_id);
  MY_FREE(instance->image_href);
  MY_FREE(instance->flavor_href);
  MY_FREE(instance->realm_href);
  MY_FREE(instance->state);
  free_action_list(&instance->actions);
  free_address_list(&instance->public_addresses);
  free_address_list(&instance->private_addresses);
}

void free_instance_list(struct deltacloud_instance **instances)
{
  struct deltacloud_instance *curr, *next;

  curr = *instances;
  while (curr != NULL) {
    next = curr->next;
    free_instance(curr);
    MY_FREE(curr);
    curr = next;
  }

  *instances = NULL;
}
