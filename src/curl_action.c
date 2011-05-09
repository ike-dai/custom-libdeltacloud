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

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include "libdeltacloud.h"
#include "curl_action.h"
#include "common.h"

static void set_curl_error(int errcode, const char *header, CURLcode res)
{
  char *errstr;
  int alloc_fail = 0;

  if (asprintf(&errstr, "%s: %s", header, curl_easy_strerror(res)) < 0) {
    errstr = "Failed to set URL header";
    alloc_fail = 1;
  }

  set_error(errcode, errstr);

  if (!alloc_fail)
    SAFE_FREE(errstr);
}

struct memory {
  char *data;
  size_t size;
};

static size_t memory_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)data;
  char *tmp;

  if (realsize == 0)
    return 0;

  tmp = realloc(mem->data, mem->size + realsize + 1);
  if (tmp == NULL)
    return 0;

  mem->data = tmp;

  memcpy(mem->data + mem->size, ptr, realsize);
  mem->size += realsize;
  mem->data[mem->size] = '\0';

  return realsize;
}

static int set_user_password(CURL *curl, const char *user, const char *password)
{
  CURLcode res;

#ifdef CURL_HAVE_USERNAME
  /* Note that we do not escape user or password, although they have been
   * supplied by the user.  From what I can tell from:
   *
   * http://www.ietf.org/rfc/rfc2617.txt
   *
   * it seems that user/password are base64 encoded, so special characters
   * should be automatically handled.
   */
  if (user != NULL) {
    res = curl_easy_setopt(curl, CURLOPT_USERNAME, user);
    if (res != CURLE_OK) {
      set_curl_error(DELTACLOUD_OOM_ERROR, "Failed to set username header",
		     res);
      return -1;
    }
  }

  if (password != NULL) {
    res = curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
    if (res != CURLE_OK) {
      set_curl_error(DELTACLOUD_OOM_ERROR, "Failed to set password header",
		     res);
      return -1;
    }
  }
#else
  if (user != NULL && password != NULL) {
    char *userpwd;

    if (asprintf(&userpwd, "%s:%s", user, password) < 0) {
      set_error(DELTACLOUD_OOM_ERROR, "Failed to allocate memory");
      return -1;
    }
    res = curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
    SAFE_FREE(userpwd);
    if (res != CURLE_OK) {
      set_curl_error(DELTACLOUD_OOM_ERROR,
		     "Failed to set username/password header", res);
      return -1;
    }
  }
#endif

  return 0;
}

int do_get_post_url(const char *url, const char *user, const char *password,
		    int post, char *data, char **returndata,
		    char **returnheader)
{
  CURL *curl;
  CURLcode res;
  struct curl_slist *reqlist = NULL;
  struct memory chunk;
  struct memory header_chunk;
  int ret = -1;
  size_t datalen;

  memset(&chunk, 0, sizeof(struct memory));
  memset(&header_chunk, 0, sizeof(struct memory));

  curl = curl_easy_init();
  if (curl == NULL) {
    set_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
	      "Failed to initialize curl library");
    return ret;
  }

  reqlist = curl_slist_append(reqlist, "Accept: application/xml");
  if (reqlist == NULL) {
    set_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
	      "Failed to create request list");
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_URL, url);
  if (res != CURLE_OK) {
    set_curl_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
		   "Failed to set URL header", res);
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, reqlist);
  if (res != CURLE_OK) {
    set_curl_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
		   "Failed to set HTTP header", res);
    goto cleanup;
  }

  if (set_user_password(curl, user, password) < 0)
    /* set_user_password already printed the error */
    goto cleanup;

  res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memory_callback);
  if (res != CURLE_OK) {
    set_curl_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
		   "Failed to set data callback", res);
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  if (res != CURLE_OK) {
    set_curl_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
		   "Failed to set data pointer", res);
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, memory_callback);
  if (res != CURLE_OK) {
    set_curl_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
		   "Failed to set header callback", res);
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&header_chunk);
  if (res != CURLE_OK) {
    set_curl_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
		   "Failed to set header pointer", res);
    goto cleanup;
  }

  if (post) {
    /* in this case, we want to do a POST; note, however, that it is possible
     * for us to do a POST with no data
     */
    res = curl_easy_setopt(curl, CURLOPT_POST, 1);
    if (res != CURLE_OK) {
      set_curl_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
		     "Failed to set header POST", res);
      goto cleanup;
    }

    /* it is imperative to set POSTFIELDSIZE so that 0-size POST transfers
     * will work
     */
    datalen = 0;
    if (data != NULL)
      datalen = strlen(data);
    res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, datalen);
    if (res != CURLE_OK) {
      set_curl_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
		     "Failed to set post field size", res);
      goto cleanup;
    }
    if (data != NULL) {
      res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
      if (res != CURLE_OK) {
	set_curl_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
		       "Failed to set header post fields", res);
	goto cleanup;
      }
    }
  }

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    set_curl_error(post ? DELTACLOUD_POST_URL_ERROR : DELTACLOUD_GET_URL_ERROR,
		   "Failed to perform transfer", res);
    goto cleanup;
  }

  ret = 0;
  if (chunk.data != NULL && returndata != NULL)
    *returndata = strdup(chunk.data);

  if (header_chunk.data != NULL && returnheader != NULL)
    *returnheader = strdup(header_chunk.data);

 cleanup:
  SAFE_FREE(chunk.data);
  SAFE_FREE(header_chunk.data);
  curl_slist_free_all(reqlist);
  curl_easy_cleanup(curl);

  return ret;
}

int delete_url(const char *url, const char *user, const char *password,
	       char **returndata)
{
  CURL *curl;
  CURLcode res;
  struct curl_slist *reqlist = NULL;
  struct memory chunk;
  int ret = -1;

  memset(&chunk, 0, sizeof(struct memory));

  curl = curl_easy_init();
  if (curl == NULL) {
    set_error(DELTACLOUD_DELETE_URL_ERROR, "Failed to initialize curl library");
    return -1;
  }

  reqlist = curl_slist_append(reqlist, "Accept: application/xml");
  if (reqlist == NULL) {
    set_error(DELTACLOUD_DELETE_URL_ERROR, "Failed to create request list");
    goto cleanup;
  }

  if (set_user_password(curl, user, password) < 0)
    /* set_user_password already printed the error */
    goto cleanup;

  res = curl_easy_setopt(curl, CURLOPT_URL, url);
  if (res != CURLE_OK) {
    set_curl_error(DELTACLOUD_DELETE_URL_ERROR, "Failed to set URL header",
		   res);
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, reqlist);
  if (res != CURLE_OK) {
    set_curl_error(DELTACLOUD_DELETE_URL_ERROR, "Failed to set HTTP header",
		   res);
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  if (res != CURLE_OK) {
    set_curl_error(DELTACLOUD_DELETE_URL_ERROR,
		   "Failed to set custom request to DELETE", res);
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memory_callback);
  if (res != CURLE_OK) {
    set_curl_error(DELTACLOUD_DELETE_URL_ERROR, "Failed to set data callback",
		   res);
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  if (res != CURLE_OK) {
    set_curl_error(DELTACLOUD_DELETE_URL_ERROR, "Failed to set data pointer",
		   res);
    goto cleanup;
  }

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    set_curl_error(DELTACLOUD_DELETE_URL_ERROR, "Failed to perform transfer",
		   res);
    SAFE_FREE(chunk.data);
  }

  ret = 0;

  if (chunk.data != NULL && returndata != NULL)
    *returndata = strdup(chunk.data);

 cleanup:
  SAFE_FREE(chunk.data);
  curl_slist_free_all(reqlist);
  curl_easy_cleanup(curl);

  return ret;
}
