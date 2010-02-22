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

char *do_curl(const char *url, const char *user, const char *password, int post,
	      char *data, int datalen)
{
  CURL *curl;
  CURLcode res;
  struct curl_slist *reqlist = NULL;
  struct memory chunk;

  curl = curl_easy_init();
  if (curl == NULL) {
    fprintf(stderr, "Failed to initialize curl\n");
    return NULL;
  }

  chunk.data = NULL;
  chunk.size = 0;

  reqlist = curl_slist_append(reqlist, "Accept: application/xml");
  if (reqlist == NULL) {
    fprintf(stderr, "Failed to create request list\n");
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_URL, url);
  if (res != CURLE_OK) {
    fprintf(stderr, "Failed to set URL header: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, reqlist);
  if (res != CURLE_OK) {
    fprintf(stderr, "Failed to set HTTP header: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  if (user != NULL) {
    res = curl_easy_setopt(curl, CURLOPT_USERNAME, user);
    if (res != CURLE_OK) {
      fprintf(stderr, "Failed to set username header: %s\n",
	      curl_easy_strerror(res));
      goto cleanup;
    }
  }

  if (password != NULL) {
    res = curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
    if (res != CURLE_OK) {
      fprintf(stderr, "Failed to set password header: %s\n",
	      curl_easy_strerror(res));
      goto cleanup;
    }
  }

  res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memory_callback);
  if (res != CURLE_OK) {
    fprintf(stderr, "Failed to set data callback: %s\n",
	    curl_easy_strerror(res));
    goto cleanup;
  }

  res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  if (res != CURLE_OK) {
    fprintf(stderr, "Failed to set data pointer: %s\n",
	    curl_easy_strerror(res));
    goto cleanup;
  }

  if (post) {
    /* in this case, we want to do a POST; note, however, that it is possible
     * for us to do a POST with no data
     */
    res = curl_easy_setopt(curl, CURLOPT_POST, 1);
    if (res != CURLE_OK) {
      fprintf(stderr, "Failed to set header POST: %s\n",
	      curl_easy_strerror(res));
      goto cleanup;
    }

    /* it is imperative to set POSTFIELDSIZE so that 0-size POST transfers
     * will work
     */
    res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, datalen);
    if (res != CURLE_OK) {
      fprintf(stderr, "Failed to set post field size: %s\n",
	      curl_easy_strerror(res));
      goto cleanup;
    }
    if (data != NULL) {
      res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
      if (res != CURLE_OK) {
	fprintf(stderr, "Failed to set header post fields: %s\n",
		curl_easy_strerror(res));
	goto cleanup;
      }
    }
  }

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "Failed to perform transfer: %s\n",
	    curl_easy_strerror(res));
    MY_FREE(chunk.data);
  }

 cleanup:
  curl_slist_free_all(reqlist);
  curl_easy_cleanup(curl);

  return chunk.data;
}
