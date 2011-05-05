## Process this file with automake to produce Makefile.in

libdeltacloudincdir = $(includedir)/libdeltacloud

libdeltacloudinc_HEADERS = libdeltacloud.h link.h instance.h realm.h image.h \
	hardware_profile.h instance_state.h storage_volume.h \
	storage_snapshot.h key.h

install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(libdeltacloudincdir)
