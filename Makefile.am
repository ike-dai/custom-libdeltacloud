## Process this file with automake to produce Makefile.in

EXTRA_DIST = libdcloudapi.spec
SUBDIRS = dcloudapi

AM_CPPFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS) -Wall -I$(builddir)/dcloudapi
libdcloudapi_la_LDFLAGS = $(LIBXML_LIBS) $(LIBCURL_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdcloudapi.pc

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libdcloudapi.la

libdcloudapi_la_SOURCES =		\
  dcloudapi/dcloudapi.h			\
  dcloudapi.c				\
  common.h				\
  common.c				\
  dcloudapi/flavor.h			\
  flavor.c				\
  geturl.h				\
  geturl.c				\
  dcloudapi/image.h			\
  image.c				\
  dcloudapi/instance.h			\
  instance.c				\
  dcloudapi/instance_state.h		\
  instance_state.c			\
  dcloudapi/link.h			\
  link.c				\
  dcloudapi/realm.h			\
  realm.c				\
  dcloudapi/storage_snapshot.h		\
  storage_snapshot.c			\
  dcloudapi/storage_volume.h		\
  storage_volume.c

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

check_PROGRAMS = dcloud-client

LDADD = $(lib_LTLIBRARIES)

generated_test_scripts = $(pathsubst %,%.sh,$(check_PROGRAMS))
TESTS = $(generated_test_scripts)

DISTCLEANFILES = $(generated_test_scripts) *~ $(pkgconfig_DATA)
