/*
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef STORAGE_SNAPSHOT_H
#define STORAGE_SNAPSHOT_H

#ifdef __cplusplus
extern "C" {
#endif

struct storage_snapshot {
  char *href;
  char *id;
  char *created;
  char *state;
  char *storage_volume_href;

  struct storage_snapshot *next;
};

int add_to_storage_snapshot_list(struct storage_snapshot **storage_snapshots,
				 const char *href, const char *id,
				 const char *created, const char *state,
				 const char *storage_volume_href);
int copy_storage_snapshot(struct storage_snapshot *dst,
			  struct storage_snapshot *src);
void print_storage_snapshot(struct storage_snapshot *storage_snapshot,
			    FILE *stream);
void print_storage_snapshot_list(struct storage_snapshot **storage_snapshots,
				 FILE *stream);
void free_storage_snapshot(struct storage_snapshot *storage_snapshot);
void free_storage_snapshot_list(struct storage_snapshot **storage_snapshots);

#ifdef __cplusplus
}
#endif

#endif
