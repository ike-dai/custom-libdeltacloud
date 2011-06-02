#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "test_common.h"

int wait_for_instance_boot(struct deltacloud_api *api, const char *instid,
			   struct deltacloud_instance *instance)
{
  int timeout = 300;
  int found = 0;

      /* wait for up to 5 minutes for the instance to show up */
  while (1) {
    fprintf(stderr, "Waiting for instance to go RUNNING, %d/300\n", timeout);

    if (deltacloud_get_instance_by_id(api, instid, instance) < 0) {
      /* FIXME: yuck.  If we got here, then we created an instance, but
       * we couldn't look it up.  That means we can't destroy it either.
       */
      fprintf(stderr, "Failed to lookup instance by id: %s\n",
	      deltacloud_get_last_error_string());
      break;
    }

    /* if the instance went to running, we are done */
    if (strcmp(instance->state, "RUNNING") == 0) {
      found = 1;
      break;
    }
    else {
      /* otherwise, see if we exceeded the timeout.  If so, destroy
       * the instance and fail
       */
      timeout--;
      if (timeout == 0) {
	fprintf(stderr, "Instance never went running, failing\n");
	deltacloud_instance_destroy(api, instance);
	break;
      }
      deltacloud_free_instance(instance);
    }

    sleep(1);
  }

  return found;
}

void print_link_list(struct deltacloud_link *links)
{
  struct deltacloud_link *link;
  struct deltacloud_feature *feature;
  struct deltacloud_feature_constraint *constraint;

  deltacloud_for_each(link, links) {
    fprintf(stderr, "Link:\n");
    fprintf(stderr, "\tRel: %s\n", link->rel);
    fprintf(stderr, "\tHref: %s\n", link->href);
    fprintf(stderr, "\tFeatures:\n");
    deltacloud_for_each(feature, link->features) {
      fprintf(stderr, "\t\t%s\n", feature->name);
      deltacloud_for_each(constraint, feature->constraints)
	fprintf(stderr, "\t\t\t%s = %s\n", constraint->name,
		constraint->value);
    }
    fprintf(stderr, "\n");
  }
}

void print_hwp(struct deltacloud_hardware_profile *hwp)
{
  struct deltacloud_property *prop;
  struct deltacloud_property_param *param;
  struct deltacloud_property_range *range;
  struct deltacloud_property_enum *enump;

  fprintf(stderr, "HWP: %s\n", hwp->id);
  fprintf(stderr, "\tName: %s\n", hwp->name);
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

void print_address_list(const char *header,
			struct deltacloud_address *addresses)
{
  struct deltacloud_address *addr;

  fprintf(stderr, "\t%s:\n", header);
  deltacloud_for_each(addr, addresses)
    fprintf(stderr, "\t\tAddress: %s\n", addr->address);
}

void print_action_list(struct deltacloud_action *actions)
{
  struct deltacloud_action *action;

  fprintf(stderr, "\tActions:\n");
  deltacloud_for_each(action, actions) {
    fprintf(stderr, "\t\tAction rel: %s, method: %s\n", action->rel,
	    action->method);
    fprintf(stderr, "\t\t\tHref: %s\n", action->href);
  }
}

