# Process this file with automake to produce Makefile.in

EXTRA_DIST = libdeltacloud.spec libdeltacloud.pc.in
SUBDIRS = include src tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdeltacloud.pc

ACLOCAL_AMFLAGS = -I m4

rpm: clean dist
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

srpm: clean dist
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ts $(distdir).tar.gz)

DISTCLEANFILES = *~ $(pkgconfig_DATA)
