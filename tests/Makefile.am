AM_CFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS) -Wall -Werror \
	-I../include/libdeltacloud -fno-strict-aliasing

check_PROGRAMS = test_api test_bucket test_driver test_firewall test_hwp \
	test_image test_instance test_instance_state test_key \
	test_loadbalancer test_param test_realm test_storage_snapshot \
	test_storage_volume

test_api_SOURCES = test_api.c test_common.c
test_api_LDADD = ../src/libdeltacloud.la

test_bucket_SOURCES = test_bucket.c test_common.c
test_bucket_LDADD = ../src/libdeltacloud.la

test_driver_SOURCES = test_driver.c test_common.c
test_driver_LDADD = ../src/libdeltacloud.la

test_firewall_SOURCES = test_firewall.c test_common.c
test_firewall_LDADD = ../src/libdeltacloud.la

test_hwp_SOURCES = test_hwp.c test_common.c
test_hwp_LDADD = ../src/libdeltacloud.la

test_image_SOURCES = test_image.c test_common.c
test_image_LDADD = ../src/libdeltacloud.la

test_instance_SOURCES = test_instance.c test_common.c
test_instance_LDADD = ../src/libdeltacloud.la

test_instance_state_SOURCES = test_instance_state.c test_common.c
test_instance_state_LDADD = ../src/libdeltacloud.la

test_key_SOURCES = test_key.c test_common.c
test_key_LDADD = ../src/libdeltacloud.la

test_loadbalancer_SOURCES = test_loadbalancer.c test_common.c
test_loadbalancer_LDADD = ../src/libdeltacloud.la

test_param_SOURCES = test_param.c test_common.c
test_param_LDADD = ../src/libdeltacloud.la

test_realm_SOURCES = test_realm.c test_common.c
test_realm_LDADD = ../src/libdeltacloud.la

test_storage_snapshot_SOURCES = test_storage_snapshot.c test_common.c
test_storage_snapshot_LDADD = ../src/libdeltacloud.la

test_storage_volume_SOURCES = test_storage_volume.c test_common.c
test_storage_volume_LDADD = ../src/libdeltacloud.la
