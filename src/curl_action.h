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

#ifndef GETURL_H
#define GETURL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <curl/curl.h>

int do_get_post_url(const char *url, const char *user, const char *password,
		    int post, char *data, struct curl_slist *inheader,
		    char **returndata, char **returnheader);

#define get_url(url, user, password, returndata) do_get_post_url(url, user, password, 0, NULL, NULL, returndata, NULL)
#define post_url(url, user, password, data, returndata, returnheader) do_get_post_url(url, user, password, 1, data, NULL, returndata, returnheader)
#define post_url_with_headers(url, user, password, inputheaders, returndata) do_get_post_url(url, user, password, 1, NULL, inputheaders, returndata, NULL)

int delete_url(const char *url, const char *user, const char *password,
	       char **returndata);

int do_multipart_post_url(const char *url, const char *user,
			  const char *password,
			  struct curl_httppost *httppost,
			  char **returndata);

int head_url(const char *url, const char *user, const char *password,
	     char **returnheader);

#ifdef __cplusplus
}
#endif

#endif
