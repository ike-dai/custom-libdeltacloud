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
#include <string.h>
#include <memory.h>
#include "common.h"
#include "metric.h"
#include "metric_value.h"
#include "curl_action.h"

int parse_metric_value_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt, void *output);

int parse_metric_xml(xmlNodePtr cur, xmlXPathContextPtr ctxt,
			      void **data)
{
  struct deltacloud_metric **metrics = (struct deltacloud_metric **)data;
  struct deltacloud_metric *thismetric;
  xmlNodePtr oldnode;
  int ret = -1;
  *metrics = NULL;
  oldnode = ctxt->node;
  cur = cur->children->next->next->next->children;
  while (cur != NULL) {
    if (cur->type == XML_ELEMENT_NODE) {
      thismetric = calloc(1, sizeof(struct deltacloud_metric));
      thismetric->name = strdup((const char *)cur->name);

      ctxt->node = cur;

      if (thismetric == NULL) {
	oom_error();
	goto cleanup;
      }

      if (parse_metric_value_xml(cur, ctxt, thismetric) < 0) {
	/* parse_one_instance is expected to have set its own error */
	SAFE_FREE(thismetric);
	goto cleanup;
      }
      /* add_to_list can't fail */

      add_to_list(metrics, struct deltacloud_metric, thismetric);
    }
    cur = cur->next;
  }

  ret = 0;

 cleanup:
  if (ret < 0)
    deltacloud_free_metric_list(metrics);
  ctxt->node = oldnode;

  return ret;
}

void deltacloud_free_metric(struct deltacloud_metric *metric)
{
  if (metric == NULL)
    return;

  SAFE_FREE(metric->href);
  SAFE_FREE(metric->name);
  deltacloud_free_metric_value_list(&metric->values);
}



void deltacloud_free_metric_list(struct deltacloud_metric **metrics)
{
  free_list(metrics, struct deltacloud_metric, deltacloud_free_metric);
}


int deltacloud_get_metrics_by_instance_id(struct deltacloud_api *api, const char *instance_id, struct deltacloud_metric **metrics)
{
  return internal_get_by_id_pp(api, instance_id, "metrics", "metric",
			    parse_metric_xml, (void **)metrics);
}

