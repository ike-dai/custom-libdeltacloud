## Process this file with automake to produce Makefile.in

EXTRA_DIST = libdcloudapi.spec
SUBDIRS = dcloudapi

AM_CPPFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS) -Wall -I$(builddir)/dcloudapi
libdcloudapi_la_LDFLAGS = $(LIBXML_LIBS) $(LIBCURL_LIBS) \
	$(VERSION_SCRIPT_FLAGS)libdcloudapi.syms -version-info 1:0:0

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdcloudapi.pc

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libdcloudapi.la

libdcloudapi_la_SOURCES =		\
  dcloudapi/dcloudapi.h			\
  dcloudapi.c				\
  common.h				\
  common.c				\
  geturl.h				\
  geturl.c				\
  dcloudapi/image.h			\
  image.c				\
  dcloudapi/hardware_profile.h		\
  hardware_profile.c			\
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
  storage_volume.c                      \
  libdcloudapi.syms

rpm: clean dist
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

check_PROGRAMS = dcloud-client

LDADD = $(lib_LTLIBRARIES)

generated_test_scripts = $(pathsubst %,%.sh,$(check_PROGRAMS))
TESTS = $(generated_test_scripts)

DISTCLEANFILES = $(generated_test_scripts) *~ $(pkgconfig_DATA)
