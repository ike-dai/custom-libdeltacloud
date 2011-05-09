#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "libdeltacloud.h"

int wait_for_instance_boot(struct deltacloud_api *api, const char *instid,
			   struct deltacloud_instance *instance);
void print_link_list(struct deltacloud_link *links);
void print_hwp(struct deltacloud_hardware_profile *hwp);
void print_address_list(const char *header,
			struct deltacloud_address *addresses);
void print_action_list(struct deltacloud_action *actions);

#endif
