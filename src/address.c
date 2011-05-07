/*
 * Copyright (C) 2011 Red Hat, Inc.
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
#include "address.h"

int parse_addresses_xml(xmlNodePtr root, xmlXPathContextPtr ctxt,
			struct deltacloud_address **addresses)
{
  struct deltacloud_address *thisaddr;
  char *address;
  xmlNodePtr oldnode, cur;
  int ret = -1;

  *addresses = NULL;

  oldnode = ctxt->node;

  ctxt->node = root;
  cur = root->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "address")) {

      address = getXPathString("string(./address)", ctxt);
      if (address != NULL) {
	thisaddr = calloc(1, sizeof(struct deltacloud_address));
	if (thisaddr == NULL) {
	  oom_error();
	  goto cleanup;
	}

	thisaddr->address = address;

	/* add_to_list can't fail */
	add_to_list(addresses, struct deltacloud_address, thisaddr);
      }
      /* address is allowed to be NULL, so skip it here */
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    free_address_list(addresses);

  return ret;
}

static void free_address(struct deltacloud_address *addr)
{
  SAFE_FREE(addr->address);
}

static int add_to_address_list(struct deltacloud_address **addresses,
			       struct deltacloud_address *address)
{
  struct deltacloud_address *oneaddress;

  oneaddress = calloc(1, sizeof(struct deltacloud_address));
  if (oneaddress == NULL)
    return -1;

  if (strdup_or_null(&oneaddress->address, address->address) < 0)
    goto error;

  add_to_list(addresses, struct deltacloud_address, oneaddress);

  return 0;

 error:
  free_address(oneaddress);
  SAFE_FREE(oneaddress);
  return -1;
}

int copy_address_list(struct deltacloud_address **dst,
		      struct deltacloud_address **src)
{
  copy_list(dst, src, struct deltacloud_address, add_to_address_list,
	    free_address_list);
}

void free_address_list(struct deltacloud_address **addresses)
{
  free_list(addresses, struct deltacloud_address, free_address);
}

