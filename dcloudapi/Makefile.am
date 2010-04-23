## Process this file with automake to produce Makefile.in

dcloudincdir = $(includedir)/dcloudapi

dcloudinc_HEADERS = dcloudapi.h link.h instance.h realm.h image.h \
	instance_state.h storage_volume.h storage_snapshot.h

install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(dcloudincdir)
