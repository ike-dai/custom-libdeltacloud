/*
 * Copyright (C) 2010,2011 Red Hat, Inc.
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
#include <string.h>
#include "libdeltacloud.h"

static void print_provider(struct deltacloud_driver_provider *provider)
{
  fprintf(stderr, "\tProvider: %s\n", provider->id);
}

static void print_driver(struct deltacloud_driver *driver)
{
  struct deltacloud_driver_provider *provider;

  fprintf(stderr, "Driver %s\n", driver->id);
  fprintf(stderr, "\tHref: %s\n", driver->href);
  fprintf(stderr, "\tName: %s\n", driver->name);
  deltacloud_for_each(provider, driver->providers)
    print_provider(provider);
}

static void print_driver_list(struct deltacloud_driver *drivers)
{
  struct deltacloud_driver *driver;

  deltacloud_for_each(driver, drivers)
    print_driver(driver);
}

static void print_key(struct deltacloud_key *key)
{
  fprintf(stderr, "Key %s\n", key->id);
  fprintf(stderr, "\tHref: %s\n", key->href);
  fprintf(stderr, "\tType: %s, State: %s\n", key->type, key->state);
  fprintf(stderr, "\tFingerprint: %s\n", key->fingerprint);
}

static void print_key_list(struct deltacloud_key *keys)
{
  struct deltacloud_key *key;

  deltacloud_for_each(key, keys)
    print_key(key);
}

static void print_instance_state(struct deltacloud_instance_state *state)
{
  struct deltacloud_instance_state_transition *transition;

  fprintf(stderr, "Instance State: %s\n", state->name);
  deltacloud_for_each(transition, state->transitions)
    fprintf(stderr, "\tTransition Action: %s, To: %s, Bool: %s\n",
	    transition->action, transition->to, transition->auto_bool);
}

static void print_instance_state_list(struct deltacloud_instance_state *states)
{
  struct deltacloud_instance_state *state;

  deltacloud_for_each(state, states)
    print_instance_state(state);
}

static void print_realm(struct deltacloud_realm *realm)
{
  fprintf(stderr, "Realm: %s\n", realm->name);
  fprintf(stderr, "\tHref: %s\n", realm->href);
  fprintf(stderr, "\tID: %s\n", realm->id);
  fprintf(stderr, "\tLimit: %s\n", realm->limit);
  fprintf(stderr, "\tState: %s\n", realm->state);
}

static void print_realm_list(struct deltacloud_realm *realms)
{
  struct deltacloud_realm *realm;

  deltacloud_for_each(realm, realms)
    print_realm(realm);
}

static void print_image(struct deltacloud_image *image)
{
  fprintf(stderr, "Image: %s\n", image->name);
  fprintf(stderr, "\tID: %s\n", image->id);
  fprintf(stderr, "\tHref: %s\n", image->href);
  fprintf(stderr, "\tDescription: %s\n", image->description);
  fprintf(stderr, "\tOwner ID: %s\n", image->owner_id);
  fprintf(stderr, "\tState: %s\n", image->state);
}

static void print_image_list(struct deltacloud_image *images)
{
  struct deltacloud_image *image;

  deltacloud_for_each(image, images)
    print_image(image);
}

static void print_link_list(struct deltacloud_link *links)
{
  struct deltacloud_link *link;
  struct deltacloud_feature *feature;

  deltacloud_for_each(link, links) {
    fprintf(stderr, "Link:\n");
    fprintf(stderr, "\tRel: %s\n", link->rel);
    fprintf(stderr, "\tHref: %s\n", link->href);
    fprintf(stderr, "\tFeatures: ");
    deltacloud_for_each(feature, link->features)
      fprintf(stderr, "%s, ", feature->name);
    fprintf(stderr, "\n");
  }
}

static void print_api(struct deltacloud_api *api)
{
  fprintf(stderr, "API %s:\n", api->url);
  fprintf(stderr, "\tUser: %s, Driver: %s, Version: %s\n", api->user,
	  api->driver, api->version);
  print_link_list(api->links);
}

static void print_hwp(struct deltacloud_hardware_profile *hwp)
{
  struct deltacloud_property *prop;
  struct deltacloud_property_param *param;
  struct deltacloud_property_range *range;
  struct deltacloud_property_enum *enump;

  fprintf(stderr, "HWP: %s\n", hwp->name);
  fprintf(stderr, "\tID: %s\n", hwp->id);
  fprintf(stderr, "\tHref: %s\n", hwp->href);
  fprintf(stderr, "\tProperties:\n");
  deltacloud_for_each(prop, hwp->properties) {
    fprintf(stderr, "\t\tName: %s, Kind: %s, Unit: %s, Value: %s\n",
	    prop->name, prop->kind, prop->unit, prop->value);
    deltacloud_for_each(param, prop->params) {
      fprintf(stderr, "\t\t\tParam name: %s\n", param->name);
      fprintf(stderr, "\t\t\t\thref: %s\n", param->href);
      fprintf(stderr, "\t\t\t\tmethod: %s\n", param->method);
      fprintf(stderr, "\t\t\t\toperation: %s\n", param->operation);
    }
    deltacloud_for_each(range, prop->ranges)
      fprintf(stderr, "\t\t\tRange first: %s, last: %s\n", range->first,
	      range->last);
    deltacloud_for_each(enump, prop->enums)
      fprintf(stderr, "\t\t\tEnum value: %s\n", enump->value);
  }
}

static void print_hwp_list(struct deltacloud_hardware_profile *profiles)
{
  struct deltacloud_hardware_profile *hwp;

  deltacloud_for_each(hwp, profiles)
    print_hwp(hwp);
}

static void print_instance(struct deltacloud_instance *instance)
{
  struct deltacloud_action *action;
  struct deltacloud_address *addr;

  fprintf(stderr, "Instance: %s\n", instance->name);
  fprintf(stderr, "\tHref: %s, ID: %s\n", instance->href, instance->id);
  fprintf(stderr, "\tOwner ID: %s\n", instance->owner_id);
  fprintf(stderr, "\tImage ID: %s, Image Href: %s\n", instance->image_id,
	  instance->image_href);
  fprintf(stderr, "\tRealm ID: %s, Realm Href: %s\n", instance->realm_id,
	  instance->realm_href);
  fprintf(stderr, "\tKey Name: %s, State: %s\n", instance->key_name,
	  instance->state);
  fprintf(stderr, "\t");
  print_hwp(&instance->hwp);
  fprintf(stderr, "\tActions:\n");
  deltacloud_for_each(action, instance->actions) {
    fprintf(stderr, "\t\tAction rel: %s, method: %s\n", action->rel,
	    action->method);
    fprintf(stderr, "\t\t\tHref: %s\n", action->href);
  }
  fprintf(stderr, "\tPublic Addresses:\n");
  deltacloud_for_each(addr, instance->public_addresses)
    fprintf(stderr, "\t\tAddress: %s\n", addr->address);
  fprintf(stderr, "\tPrivate Addresses:\n");
  deltacloud_for_each(addr, instance->private_addresses)
    fprintf(stderr, "\t\tAddress: %s\n", addr->address);
}

static void print_instance_list(struct deltacloud_instance *instances)
{
  struct deltacloud_instance *instance;

  deltacloud_for_each(instance, instances)
    print_instance(instance);
}

static void print_storage_volume(struct deltacloud_storage_volume *volume)
{
  fprintf(stderr, "Storage Volume: %s\n", volume->id);
  fprintf(stderr, "\tHref: %s\n", volume->href);
  fprintf(stderr, "\tCreated: %s, State: %s\n", volume->created, volume->state);
  fprintf(stderr, "\tCapacity: Unit: %s, Size: %s\n", volume->capacity.unit,
	  volume->capacity.size);
  fprintf(stderr, "\tDevice: %s\n", volume->device);
  fprintf(stderr, "\tInstance Href: %s\n", volume->instance_href);
  fprintf(stderr, "\tRealm ID: %s\n", volume->realm_id);
  fprintf(stderr, "\tMount: %s\n", volume->mount.instance_id);
  fprintf(stderr, "\t\tInstance Href: %s\n", volume->mount.instance_href);
  fprintf(stderr, "\t\tDevice Name: %s\n", volume->mount.device_name);
}

static void print_storage_volume_list(struct deltacloud_storage_volume *volumes)
{
  struct deltacloud_storage_volume *volume;

  deltacloud_for_each(volume, volumes)
    print_storage_volume(volume);
}

static void print_storage_snapshot(struct deltacloud_storage_snapshot *snapshot)
{
  fprintf(stderr, "Snapshot: %s\n", snapshot->id);
  fprintf(stderr, "\tHref: %s\n", snapshot->href);
  fprintf(stderr, "\tCreated: %s, State: %s\n", snapshot->created,
	  snapshot->state);
  fprintf(stderr, "\tStorage Volume Href: %s\n", snapshot->storage_volume_href);
  fprintf(stderr, "\tStorage Volume ID: %s\n", snapshot->storage_volume_id);
}

static void print_storage_snapshot_list(struct deltacloud_storage_snapshot *snapshots)
{
  struct deltacloud_storage_snapshot *snapshot;

  deltacloud_for_each(snapshot, snapshots)
    print_storage_snapshot(snapshot);
}

int main(int argc, char *argv[])
{
  struct deltacloud_api api;
  struct deltacloud_instance *instances;
  struct deltacloud_image *images;
  struct deltacloud_realm *realms;
  struct deltacloud_instance_state *instance_states;
  struct deltacloud_storage_volume *storage_volumes;
  struct deltacloud_storage_snapshot *storage_snapshots;
  struct deltacloud_hardware_profile *profiles;
  struct deltacloud_instance_state instance_state;
  struct deltacloud_realm realm;
  struct deltacloud_hardware_profile hwp;
  struct deltacloud_image image;
  struct deltacloud_instance instance;
  struct deltacloud_storage_volume storage_volume;
  struct deltacloud_storage_snapshot storage_snapshot;
  struct deltacloud_key *keys;
  struct deltacloud_key key;
  struct deltacloud_driver *drivers;
  struct deltacloud_driver driver;
  char *instid;
  struct deltacloud_create_parameter stackparams[2];
  struct deltacloud_create_parameter *heapparams[2];
  int ret = 3;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <url> <user> <password>\n", argv[0]);
    return 1;
  }

  fprintf(stderr, "--------------LINKS--------------------------\n");
  if (deltacloud_initialize(&api, argv[1], argv[2], argv[3]) < 0) {
    fprintf(stderr, "Failed to find links for the API: %s\n",
	    deltacloud_get_last_error_string());
    return 2;
  }
  print_api(&api);

  fprintf(stderr, "---------------DRIVERS-------------------------\n");
  if (deltacloud_get_drivers(&api, &drivers) < 0) {
    fprintf(stderr, "Failed to get drivers: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  print_driver_list(drivers);

  if (drivers != NULL) {
    /* here we use the first driver from the list above */
    if (deltacloud_get_driver_by_id(&api, drivers->id, &driver) < 0) {
      fprintf(stderr, "Failed to get driver: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_driver_list(&drivers);
      goto cleanup;
    }
    print_driver(&driver);
    deltacloud_free_driver(&driver);
  }

  deltacloud_free_driver_list(&drivers);

  fprintf(stderr, "--------------HARDWARE PROFILES--------------\n");
  if (deltacloud_get_hardware_profiles(&api, &profiles) < 0) {
    fprintf(stderr, "Failed to get_hardware_profiles: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  print_hwp_list(profiles);

  if (profiles != NULL) {
    /* here we use the first hardware profile from the list above */
    if (deltacloud_get_hardware_profile_by_id(&api, profiles->id, &hwp) < 0) {
      fprintf(stderr, "Failed to get hardware profile by id: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_hardware_profile_list(&profiles);
      goto cleanup;
    }
    print_hwp(&hwp);
    deltacloud_free_hardware_profile(&hwp);
  }

  deltacloud_free_hardware_profile_list(&profiles);

  fprintf(stderr, "--------------IMAGES-------------------------\n");
  if (deltacloud_get_images(&api, &images) < 0) {
    fprintf(stderr, "Failed to get_images: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  print_image_list(images);

  if (images != NULL) {
    /* here we use the first image from the list above */
    if (deltacloud_get_image_by_id(&api, images->id, &image) < 0) {
      fprintf(stderr, "Failed to get image by id: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_image_list(&images);
      goto cleanup;
    }
    print_image(&image);
    deltacloud_free_image(&image);
  }

  deltacloud_free_image_list(&images);

  fprintf(stderr, "--------------REALMS-------------------------\n");
  if (deltacloud_get_realms(&api, &realms) < 0) {
    fprintf(stderr, "Failed to get_realms: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  print_realm_list(realms);

  if (realms != NULL) {
    /* here we use the first realm from the list above */
    if (deltacloud_get_realm_by_id(&api, realms->id, &realm) < 0) {
      fprintf(stderr, "Failed to get realm by id: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_realm_list(&realms);
      goto cleanup;
    }
    print_realm(&realm);
    deltacloud_free_realm(&realm);
  }

  deltacloud_free_realm_list(&realms);

  fprintf(stderr, "--------------INSTANCE STATES----------------\n");
  if (deltacloud_get_instance_states(&api, &instance_states) < 0) {
    fprintf(stderr, "Failed to get_instance_states: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  print_instance_state_list(instance_states);

  if (instance_states != NULL) {
    /* here we use the first instance_state from the list above */
    if (deltacloud_get_instance_state_by_name(&api, instance_states->name,
					      &instance_state) < 0) {
      fprintf(stderr, "Failed to get instance_state: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_instance_state_list(&instance_states);
      goto cleanup;
    }
    print_instance_state(&instance_state);
    deltacloud_free_instance_state(&instance_state);
  }

  deltacloud_free_instance_state_list(&instance_states);

  fprintf(stderr, "--------------INSTANCES---------------------\n");
  if (deltacloud_get_instances(&api, &instances) < 0) {
    fprintf(stderr, "Failed to get_instances: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  print_instance_list(instances);

  if (instances != NULL) {
    /* here we use the first instance from the list above */
    if (deltacloud_get_instance_by_id(&api, instances->id, &instance) < 0) {
      fprintf(stderr, "Failed to get instance by id: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_instance_list(&instances);
      goto cleanup;
    }
    print_instance(&instance);
    deltacloud_free_instance(&instance);
  }

  deltacloud_free_instance_list(&instances);

  fprintf(stderr, "--------------STORAGE VOLUMES---------------\n");
  if (deltacloud_get_storage_volumes(&api, &storage_volumes) < 0) {
    fprintf(stderr, "Failed to get_storage_volumes: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  print_storage_volume_list(storage_volumes);

  if (storage_volumes != NULL) {
    /* here we use the first storage volume from the list above */
    if (deltacloud_get_storage_volume_by_id(&api, storage_volumes->id,
					    &storage_volume) < 0) {
      fprintf(stderr, "Failed to get storage volume by ID: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_storage_volume_list(&storage_volumes);
      goto cleanup;
    }
    print_storage_volume(&storage_volume);
    deltacloud_free_storage_volume(&storage_volume);
  }

  deltacloud_free_storage_volume_list(&storage_volumes);

  fprintf(stderr, "--------------STORAGE SNAPSHOTS-------------\n");
  if (deltacloud_get_storage_snapshots(&api, &storage_snapshots) < 0) {
    fprintf(stderr, "Failed to get_storage_snapshots: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  print_storage_snapshot_list(storage_snapshots);

  if (storage_snapshots != NULL) {
    /* here we use the first storage snapshot from the list above */
    if (deltacloud_get_storage_snapshot_by_id(&api, storage_snapshots->id,
					      &storage_snapshot) < 0) {
      fprintf(stderr, "Failed to get storage_snapshot by ID: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_storage_snapshot_list(&storage_snapshots);
      goto cleanup;
    }
    print_storage_snapshot(&storage_snapshot);
    deltacloud_free_storage_snapshot(&storage_snapshot);
  }

  deltacloud_free_storage_snapshot_list(&storage_snapshots);

  fprintf(stderr, "--------------KEYS----------------------------\n");
  if (deltacloud_get_keys(&api, &keys) < 0) {
    fprintf(stderr, "Failed to get keys: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  print_key_list(keys);

  if (keys != NULL) {
    /* here we use the first key from the list above */
    if (deltacloud_get_key_by_id(&api, keys->id, &key) < 0) {
      fprintf(stderr, "Failed to get key by ID: %s\n",
	      deltacloud_get_last_error_string());
      deltacloud_free_key_list(&keys);
      goto cleanup;
    }
    print_key(&key);
    deltacloud_free_key(&key);
  }

  deltacloud_free_key_list(&keys);

  if (deltacloud_create_key(&api, "testkey", NULL, 0) < 0) {
    fprintf(stderr, "Failed to create key: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }

  if (deltacloud_get_key_by_id(&api, "testkey", &key) < 0) {
    fprintf(stderr, "Failed to retrieve just created key: %s\n",
	    deltacloud_get_last_error_string());
  }
  print_key(&key);
  deltacloud_key_destroy(&api, &key);
  deltacloud_free_key(&key);

  fprintf(stderr, "--------------CREATE INSTANCE PARAMS--------\n");
  if (deltacloud_prepare_parameter(&stackparams[0], "name", "foo") < 0) {
    fprintf(stderr, "Failed to prepare stack parameter 0: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  if (deltacloud_prepare_parameter(&stackparams[1], "keyname", "chris") < 0) {
    deltacloud_free_parameter_value(&stackparams[0]);
    fprintf(stderr, "Failed to prepare stack parameter 1: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_free_parameter_value(&stackparams[0]);
  deltacloud_free_parameter_value(&stackparams[1]);

  heapparams[0] = deltacloud_allocate_parameter("name", "foo");
  if (heapparams[0] == NULL) {
    fprintf(stderr, "Failed to prepare heap parameter 0: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  heapparams[1] = deltacloud_allocate_parameter("keyname", "chris");
  if (heapparams[1] == NULL) {
    deltacloud_free_parameter(heapparams[0]);
    fprintf(stderr, "Failed to prepare heap parameter 1: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_free_parameter(heapparams[0]);
  deltacloud_free_parameter(heapparams[1]);

  fprintf(stderr, "--------------CREATE INSTANCE---------------\n");
  deltacloud_prepare_parameter(&stackparams[0], "name", "foo");
  if (deltacloud_create_instance(&api, "img3", stackparams, 1, &instid) < 0) {
    deltacloud_free_parameter_value(&stackparams[0]);
    fprintf(stderr, "Failed to create_instance: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }
  deltacloud_free_parameter_value(&stackparams[0]);

  fprintf(stderr, "Successfully created instance with ID %s\n", instid);

  if (deltacloud_get_instance_by_id(&api, instid, &instance) < 0) {
    free(instid);
    fprintf(stderr, "Failed to get instance by id: %s\n",
	    deltacloud_get_last_error_string());
    goto cleanup;
  }

  free(instid);

  print_instance(&instance);

  if (deltacloud_instance_stop(&api, &instance) < 0)
    fprintf(stderr, "Failed to stop instance: %s\n",
	    deltacloud_get_last_error_string());
  if (deltacloud_instance_start(&api, &instance) < 0)
    fprintf(stderr, "Failed to start instance: %s\n",
	    deltacloud_get_last_error_string());
  if (deltacloud_instance_reboot(&api, &instance) < 0)
    fprintf(stderr, "Failed to reboot instance: %s\n",
	    deltacloud_get_last_error_string());
  if (deltacloud_instance_destroy(&api, &instance) < 0)
    fprintf(stderr, "Failed to destroy instance: %s\n",
	    deltacloud_get_last_error_string());
  deltacloud_free_instance(&instance);

  ret = 0;

 cleanup:
  deltacloud_free(&api);

  return ret;
}
