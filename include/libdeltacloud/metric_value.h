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

#ifndef LIBDELTACLOUD_METRIC_VALUE_H
#define LIBDELTACLOUD_METRIC_VALUE_H

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_metric_value {
  char *unit;
  char *minimum;
  char *maximum;
  char *samples;
  char *average;
  struct deltacloud_metric_value *next;
};

void deltacloud_free_metric_value(struct deltacloud_metric_value *metric_value);
void deltacloud_free_metric_value_list(struct deltacloud_metric_value **metric_values);

#ifdef __cplusplus
}
#endif

#endif
