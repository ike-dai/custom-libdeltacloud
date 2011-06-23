## Process this file with automake to produce Makefile.in

libdeltacloudincdir = $(includedir)/libdeltacloud

libdeltacloudinc_HEADERS = action.h address.h bucket.h driver.h firewall.h \
	hardware_profile.h image.h instance.h instance_state.h key.h \
	libdeltacloud.h link.h loadbalancer.h realm.h storage_snapshot.h \
	storage_volume.h

install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(libdeltacloudincdir)
