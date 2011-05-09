# Process this file with automake to produce Makefile.in

EXTRA_DIST = libdeltacloud.spec libdeltacloud.pc.in
SUBDIRS = include src tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdeltacloud.pc

ACLOCAL_AMFLAGS = -I m4

rpm: clean dist
	rpmbuild -ta $(distdir).tar.gz

srpm: clean dist
	rpmbuild -ts $(distdir).tar.gz

signed-rpm: clean dist
	rpmbuild -ta --sign $(distdir).tar.gz

signed-tarball: clean dist
	gpg --detach-sign --armor -o $(distdir).tar.gz.sign $(distdir).tar.gz

release: signed-rpm signed-tarball

DISTCLEANFILES = *~ $(pkgconfig_DATA)
