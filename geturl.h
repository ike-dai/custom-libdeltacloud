#ifndef GETURL_H
#define GETURL_H

char *do_curl(const char *url, const char *user, const char *password,
	      char *data, int datalen);

#define get_url(url, user, password) do_curl(url, user, password, NULL, 0)
#define post_url(url, user, password, data, datalen) do_curl(url, user, password, data, datalen)

#endif
