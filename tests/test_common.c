#include <stdio.h>
#include <stdlib.h>
#include "test_common.h"

void print_link_list(struct deltacloud_link *links)
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

