AM_CFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS) -Wall -Werror \
	-I../include/libdeltacloud -fno-strict-aliasing

check_PROGRAMS = libdeltacloud-client

libdeltacloud_client_SOURCES = libdeltacloud-client.c
libdeltacloud_client_LDADD = ../src/libdeltacloud.la
