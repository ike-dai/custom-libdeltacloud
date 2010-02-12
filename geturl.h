#ifndef GETURL_H
#define GETURL_H

char *do_curl(char *url, char *user, char *password, char *data, int datalen);

#define get_url(url, user, password) do_curl(url, user, password, NULL, 0)
#define post_url(url, user, password, data, datalen) do_curl(url, user, password, data, datalen)

#endif
