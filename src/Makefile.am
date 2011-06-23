EXTRA_DIST = libdeltacloud.syms

AM_CFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS) -Wall -Werror \
	-I../include/libdeltacloud -fno-strict-aliasing

libdeltacloud_la_LDFLAGS = $(LIBXML_LIBS) $(LIBCURL_LIBS) -lpthread \
	$(VERSION_SCRIPT_FLAGS)libdeltacloud.syms -version-info 6:0:0

lib_LTLIBRARIES = libdeltacloud.la

libdeltacloud_la_SOURCES =			\
  libdeltacloud.c				\
  common.h					\
  common.c					\
  curl_action.h					\
  curl_action.c					\
  image.c					\
  hardware_profile.c				\
  instance.c					\
  instance_state.c				\
  link.c					\
  realm.c					\
  storage_snapshot.c				\
  storage_volume.c                      	\
  key.c						\
  driver.c					\
  action.c					\
  address.c					\
  loadbalancer.c				\
  bucket.c 					\
  firewall.c

LDADD = $(lib_LTLIBRARIES)
