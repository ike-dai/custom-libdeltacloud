# Process this file with automake to produce Makefile.in

EXTRA_DIST = libdeltacloud.spec build-aux m4
SUBDIRS = include/libdeltacloud

AM_CPPFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS) -Wall -Iinclude/libdeltacloud -fno-strict-aliasing -Werror
libdeltacloud_la_LDFLAGS = $(LIBXML_LIBS) $(LIBCURL_LIBS) -lpthread \
	$(VERSION_SCRIPT_FLAGS)libdeltacloud.syms -version-info 2:0:0

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdeltacloud.pc

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libdeltacloud.la

libdeltacloud_la_SOURCES =			\
  libdeltacloud.c				\
  include/libdeltacloud/libdeltacloud.h		\
  common.h					\
  common.c					\
  curl_action.h					\
  curl_action.c					\
  include/libdeltacloud/image.h			\
  image.c					\
  include/libdeltacloud/hardware_profile.h	\
  hardware_profile.c				\
  include/libdeltacloud/instance.h		\
  instance.c					\
  include/libdeltacloud/instance_state.h	\
  instance_state.c				\
  include/libdeltacloud/link.h			\
  link.c					\
  include/libdeltacloud/realm.h			\
  realm.c					\
  include/libdeltacloud/storage_snapshot.h	\
  storage_snapshot.c				\
  include/libdeltacloud/storage_volume.h	\
  storage_volume.c                      	\
  libdeltacloud.syms

rpm: clean dist
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

check_PROGRAMS = libdeltacloud-client

LDADD = $(lib_LTLIBRARIES)

generated_test_scripts = $(check_PROGRAMS)
TESTS = $(generated_test_scripts)

DISTCLEANFILES = $(generated_test_scripts) *~ $(pkgconfig_DATA)
