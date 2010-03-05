AM_CPPFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS)
libdcloudapi_la_LDFLAGS = $(LIBXML_LIBS) $(LIBCURL_LIBS)

lib_LTLIBRARIES = libdcloudapi.la

libdcloudapi_la_SOURCES =	\
  dcloudapi.h			\
  dcloudapi.c			\
  common.h			\
  common.c			\
  flavor.h			\
  flavor.c			\
  geturl.h			\
  geturl.c			\
  image.h			\
  image.c			\
  instance.h			\
  instance.c			\
  instance_state.h		\
  instance_state.c		\
  link.h			\
  link.c			\
  realm.h			\
  realm.c			\
  storage_snapshot.h		\
  storage_snapshot.c		\
  storage_volume.h		\
  storage_volume.c

check_PROGRAMS = dcloud-client

LDADD = $(lib_LTLIBRARIES)

generated_test_scripts = $(pathsubst %,%.sh,$(check_PROGRAMS))
TESTS = $(generated_test_scripts)

DISTCLEANFILES = $(generated_test_scripts) *~
