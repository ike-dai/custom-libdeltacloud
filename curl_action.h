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

#ifndef GETURL_H
#define GETURL_H

#ifdef __cplusplus
extern "C" {
#endif

char *do_get_post_url(const char *url, const char *user, const char *password,
		      int post, char *data, int datalen);

#define get_url(url, user, password) do_get_post_url(url, user, password, 0, NULL, 0)
#define post_url(url, user, password, data, datalen) do_get_post_url(url, user, password, 1, data, datalen)

char *delete_url(const char *url, const char *user, const char *password);

#ifdef __cplusplus
}
#endif

#endif
