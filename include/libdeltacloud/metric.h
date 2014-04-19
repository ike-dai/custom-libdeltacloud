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

#ifndef LIBDELTACLOUD_METRIC_H
#define LIBDELTACLOUD_METRIC_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_metric {
  char *href;
  char *name; 
  struct deltacloud_metric_value *values;
  struct deltacloud_metric *next;
};

#define deltacloud_supports_metrics(api) deltacloud_has_link(api, "metrics")
int deltacloud_get_metrics_by_instance_id(struct deltacloud_api *api, const char *instance_id, struct deltacloud_metric **metrics);
void deltacloud_free_metric(struct deltacloud_metric *metric);
void deltacloud_free_metric_list(struct deltacloud_metric **metrics);
#ifdef __cplusplus
}
#endif

#endif
