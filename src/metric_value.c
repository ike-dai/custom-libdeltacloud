/*
 * Copyright (C) 2014 Daisuke Ikeda
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
 * Author: Daisuke Ikeda <dai.ikd123@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "common.h"
#include "metric_value.h"


int parse_one_metric_value(xmlNodePtr cur, xmlXPathContextPtr ctxt, struct deltacloud_metric_value **output)
{
  struct deltacloud_metric_value **thisvalues = (struct deltacloud_metric_value **)output;
  struct deltacloud_metric_value *thisvalue;
  xmlNodePtr oldnode;
  int ret = -1;
  char *name;

  oldnode = cur;
  thisvalue = calloc(1, sizeof(struct deltacloud_metric_value));

  cur = cur->children;
  if (cur == NULL) {
    thisvalue = NULL;
  }
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
        STREQ((const char *)cur->name, "property")) {
      ctxt->node = cur;
      name = (char *)xmlGetProp(cur, BAD_CAST "name");
      if (strcmp(name, "unit") == 0){
        thisvalue->unit = (char *)xmlGetProp(cur, BAD_CAST "value");
      }
      if (strcmp(name, "minimum") == 0){
        thisvalue->minimum = (char *)xmlGetProp(cur, BAD_CAST "value");
      }
      if (strcmp(name, "maximum") == 0){
        thisvalue->maximum = (char *)xmlGetProp(cur, BAD_CAST "value");
      }
      if (strcmp(name, "samples") == 0){
        thisvalue->samples = (char *)xmlGetProp(cur, BAD_CAST "value");
      }
      if (strcmp(name, "average") == 0){
        thisvalue->average = (char *)xmlGetProp(cur, BAD_CAST "value");
      }
    }
    cur = cur->next;
  }
  add_to_list(thisvalues, struct deltacloud_metric_value, thisvalue);
  ret = 0;
  goto cleanup; 
  cleanup:  
    if (ret < 0)
      deltacloud_free_metric_value_list(thisvalues);
    ctxt->node = oldnode;

  return ret;
}

int parse_metric_value_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void *output)
{
  struct deltacloud_metric *thismetric = (struct deltacloud_metric *)output;
  xmlNodePtr oldnode;
  int ret = -1;
  oldnode = ctxt->node;
  cur = cur->children;
  thismetric->values = NULL;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE &&
        STREQ((const char *)cur->name, "sample")) {
      ctxt->node = cur;
      if (parse_one_metric_value(cur, ctxt, &(thismetric->values)) < 0) {
        SAFE_FREE(thismetric);
        goto cleanup;
      }

    }
    if (cur->name != NULL )
    {
      cur = cur->next;
    }
  }
  ret = 0;
 
  cleanup:  
    ctxt->node = oldnode;
  return ret;
}

void deltacloud_free_metric_value(struct deltacloud_metric_value *metric_value)
{
  if (metric_value == NULL)
    return;

  SAFE_FREE(metric_value->unit);
  SAFE_FREE(metric_value->minimum);
  SAFE_FREE(metric_value->maximum);
  SAFE_FREE(metric_value->samples);
  SAFE_FREE(metric_value->average);
}

void deltacloud_free_metric_value_list(struct deltacloud_metric_value **metric_values)
{
  free_list(metric_values, struct deltacloud_metric_value, deltacloud_free_metric_value);
}
