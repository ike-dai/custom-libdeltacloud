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
#include "common.h"
#include "image.h"

static int parse_one_image(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			   void *output)
{
  struct deltacloud_image *thisimage = (struct deltacloud_image *)output;

  memset(thisimage, 0, sizeof(struct deltacloud_image));

  thisimage->href = (char *)xmlGetProp(cur, BAD_CAST "href");
  thisimage->id = (char *)xmlGetProp(cur, BAD_CAST "id");
  thisimage->description = getXPathString("string(./description)", ctxt);
  thisimage->architecture = getXPathString("string(./architecture)", ctxt);
  thisimage->owner_id = getXPathString("string(./owner_id)", ctxt);
  thisimage->name = getXPathString("string(./name)", ctxt);
  thisimage->state = getXPathString("string(./state)", ctxt);

  return 0;
}

static int parse_image_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			   void **data)
{
  struct deltacloud_image **images = (struct deltacloud_image **)data;
  struct deltacloud_image *thisimage;
  xmlNodePtr oldnode;
  int ret = -1;

  oldnode = ctxt->node;

  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
	STREQ((const char *)cur->name, "image")) {

      ctxt->node = cur;

      thisimage = calloc(1, sizeof(struct deltacloud_image));
      if (thisimage == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_one_image(cur, ctxt, thisimage) < 0) {
	/* parse_one_image is expected to have set its own error */
	SAFE_FREE(thisimage);
	goto cleanup;
      }

      /* add_to_list can't fail */
      add_to_list(images, struct deltacloud_image, thisimage);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  ctxt->node = oldnode;
  if (ret < 0)
    deltacloud_free_image_list(images);

  return ret;
}

int deltacloud_get_images(struct deltacloud_api *api,
			  struct deltacloud_image **images)
{
  return internal_get(api, "images", "images", parse_image_xml,
		      (void **)images);
}

int deltacloud_get_image_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_image *image)
{
  return internal_get_by_id(api, id, "images", "image", parse_one_image, image);
}

int deltacloud_create_image(struct deltacloud_api *api, const char *instance_id,
			    struct deltacloud_create_parameter *params,
			    int params_length)
{
  struct deltacloud_create_parameter *internal_params;
  int ret;
  int pos;

  if (!valid_api(api) || !valid_arg(instance_id))
    return -1;

  internal_params = calloc(params_length + 1,
			   sizeof(struct deltacloud_create_parameter));
  if (internal_params == NULL) {
    oom_error();
    return -1;
  }

  pos = copy_parameters(internal_params, params, params_length);
  if (pos < 0)
    /* copy_parameters already set the error */
    goto cleanup;

  if (deltacloud_prepare_parameter(&internal_params[pos++], "instance_id",
				   instance_id) < 0)
    /* deltacloud_prepare_parameter already set the error */
    goto cleanup;

  if (internal_create(api, "images", internal_params, pos, NULL) < 0)
    goto cleanup;

  ret = 0;

 cleanup:
  free_parameters(internal_params, pos);
  SAFE_FREE(internal_params);

  return ret;
}

void deltacloud_free_image(struct deltacloud_image *image)
{
  if (image == NULL)
    return;

  SAFE_FREE(image->href);
  SAFE_FREE(image->id);
  SAFE_FREE(image->description);
  SAFE_FREE(image->architecture);
  SAFE_FREE(image->owner_id);
  SAFE_FREE(image->name);
  SAFE_FREE(image->state);
}

void deltacloud_free_image_list(struct deltacloud_image **images)
{
  free_list(images, struct deltacloud_image, deltacloud_free_image);
}
