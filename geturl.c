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

  if (realsize == 0)
    return 0;

  mem->data = realloc(mem->data, mem->size + realsize + 1);
  if (mem->data == NULL)
    return 0;

  memcpy(mem->data + mem->size, ptr, realsize);
  mem->size += realsize;
  mem->data[mem->size] = '\0';

  return realsize;
}

char *do_curl(const char *url, const char *user, const char *password,
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

  reqlist = curl_slist_append(reqlist, "Accept: text/xml");
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

  if (data != NULL) {
    /* in this case, we want to do a POST, so add a few extra fields */
    res = curl_easy_setopt(curl, CURLOPT_POST, 1);
    if (res != CURLE_OK) {
      fprintf(stderr, "Failed to set header POST: %s\n",
	      curl_easy_strerror(res));
      goto cleanup;
    }

    res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    if (res != CURLE_OK) {
      fprintf(stderr, "Failed to set header post fields: %s\n",
	      curl_easy_strerror(res));
      goto cleanup;
    }

    res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, datalen);
    if (res != CURLE_OK) {
      fprintf(stderr, "Failed to set post field size: %s\n",
	      curl_easy_strerror(res));
      goto cleanup;
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
