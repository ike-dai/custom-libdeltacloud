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

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "common.h"

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
  if (user != NULL) {
    res = curl_easy_setopt(curl, CURLOPT_USERNAME, user);
    if (res != CURLE_OK) {
      dcloudprintf("Failed to set username header: %s\n",
		   curl_easy_strerror(res));
      return -1;
    }
  }

  if (password != NULL) {
    res = curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
    if (res != CURLE_OK) {
      dcloudprintf("Failed to set password header: %s\n",
		   curl_easy_strerror(res));
      return -1;
    }
  }
#else
  if (user != NULL && password != NULL) {
    char *userpwd;

    if (asprintf(&userpwd, "%s:%s", user, password) < 0) {
      dcloudprintf("Failed to allocate memory for userpwd\n");
      return -1;
    }
    res = curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
    SAFE_FREE(userpwd);
    if (res != CURLE_OK) {
      dcloudprintf("Failed to set username/password header: %s\n",
                   curl_easy_strerror(res));
      return -1;
    }
  }
#endif

  return 0;
}

char *do_get_post_url(const char *url, const char *user, const char *password,
		      int post, char *data, int datalen)
{
  CURL *curl;
  CURLcode res;
  struct curl_slist *reqlist = NULL;
  struct memory chunk;

  curl = curl_easy_init();
  if (curl == NULL) {
    dcloudprintf("Failed to initialize curl\n");
    return NULL;
  }

  chunk.data = NULL;
  chunk.size = 0;

  reqlist = curl_slist_append(reqlist, "Accept: application/xml");
  if (reqlist == NULL) {
    dcloudprintf("Failed to create request list\n");
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_URL, url);
  if (res != CURLE_OK) {
    dcloudprintf("Failed to set URL header: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, reqlist);
  if (res != CURLE_OK) {
    dcloudprintf("Failed to set HTTP header: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  if (set_user_password(curl, user, password) < 0)
    /* set_user_password already printed the error */
    goto cleanup;

  res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memory_callback);
  if (res != CURLE_OK) {
    dcloudprintf("Failed to set data callback: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  if (res != CURLE_OK) {
    dcloudprintf("Failed to set data pointer: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  if (post) {
    /* in this case, we want to do a POST; note, however, that it is possible
     * for us to do a POST with no data
     */
    res = curl_easy_setopt(curl, CURLOPT_POST, 1);
    if (res != CURLE_OK) {
      dcloudprintf("Failed to set header POST: %s\n", curl_easy_strerror(res));
      goto cleanup;
    }

    /* it is imperative to set POSTFIELDSIZE so that 0-size POST transfers
     * will work
     */
    res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, datalen);
    if (res != CURLE_OK) {
      dcloudprintf("Failed to set post field size: %s\n",
		   curl_easy_strerror(res));
      goto cleanup;
    }
    if (data != NULL) {
      res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
      if (res != CURLE_OK) {
	dcloudprintf("Failed to set header post fields: %s\n",
		     curl_easy_strerror(res));
	goto cleanup;
      }
    }
  }

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    dcloudprintf("Failed to perform transfer: %s\n", curl_easy_strerror(res));
    SAFE_FREE(chunk.data);
  }

 cleanup:
  curl_slist_free_all(reqlist);
  curl_easy_cleanup(curl);

  return chunk.data;
}

int delete_url(const char *url, const char *user, const char *password)
{
  CURL *curl;
  CURLcode res;
  struct curl_slist *reqlist = NULL;
  int ret = -1;

  curl = curl_easy_init();
  if (curl == NULL) {
    dcloudprintf("Failed to initialize curl\n");
    return -1;
  }

  reqlist = curl_slist_append(reqlist, "Accept: application/xml");
  if (reqlist == NULL) {
    dcloudprintf("Failed to create request list\n");
    goto cleanup;
  }

  if (set_user_password(curl, user, password) < 0)
    /* set_user_password already printed the error */
    goto cleanup;

  res = curl_easy_setopt(curl, CURLOPT_URL, url);
  if (res != CURLE_OK) {
    dcloudprintf("Failed to set URL header: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, reqlist);
  if (res != CURLE_OK) {
    dcloudprintf("Failed to set HTTP header: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  if (res != CURLE_OK) {
    dcloudprintf("Failed to set custom request to DELETE: %s\n",
		 curl_easy_strerror(res));
    goto cleanup;
  }

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    dcloudprintf("Failed to perform transfer: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  ret = 0;

 cleanup:
  curl_slist_free_all(reqlist);
  curl_easy_cleanup(curl);

  return ret;
}