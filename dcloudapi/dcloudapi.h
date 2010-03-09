#ifndef DCLOUD_API_H
#define DCLOUD_API_H

#include "link.h"
#include "instance.h"
#include "realm.h"
#include "flavor.h"
#include "image.h"
#include "instance_state.h"
#include "storage_volume.h"
#include "storage_snapshot.h"

#ifdef __cplusplus
extern "C" {
#endif

struct deltacloud_api {
  char *url;
  char *user;
  char *password;

  struct deltacloud_link *links;
};

int deltacloud_initialize(struct deltacloud_api *api, char *url, char *user,
			  char *password);

int deltacloud_get_instances(struct deltacloud_api *api,
			     struct deltacloud_instance **instances);
int deltacloud_get_instance_by_id(struct deltacloud_api *api, const char *id,
				  struct deltacloud_instance *instance);
int deltacloud_get_instance_by_name(struct deltacloud_api *api,
				    const char *name,
				    struct deltacloud_instance *instance);

int deltacloud_get_realms(struct deltacloud_api *api,
			  struct deltacloud_realm **realms);
int deltacloud_get_realm_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_realm *realm);

int deltacloud_get_flavors(struct deltacloud_api *api,
			   struct deltacloud_flavor **flavors);
int deltacloud_get_flavor_by_id(struct deltacloud_api *api, const char *id,
				struct deltacloud_flavor *flavor);
int deltacloud_get_flavor_by_uri(struct deltacloud_api *api, const char *url,
				 struct deltacloud_flavor *flavor);

int deltacloud_get_images(struct deltacloud_api *api,
			  struct deltacloud_image **images);
int deltacloud_get_image_by_id(struct deltacloud_api *api, const char *id,
			       struct deltacloud_image *image);

int deltacloud_get_instance_states(struct deltacloud_api *api,
				   struct deltacloud_instance_state **instance_states);
int deltacloud_get_instance_state_by_name(struct deltacloud_api *api,
					  const char *name,
					  struct deltacloud_instance_state *instance_state);

int deltacloud_get_storage_volumes(struct deltacloud_api *api,
				   struct deltacloud_storage_volume **storage_volumes);
int deltacloud_get_storage_volume_by_id(struct deltacloud_api *api,
					const char *id,
					struct deltacloud_storage_volume *storage_volume);

int deltacloud_get_storage_snapshots(struct deltacloud_api *api,
				     struct deltacloud_storage_snapshot **storage_snapshots);
int deltacloud_get_storage_snapshot_by_id(struct deltacloud_api *api,
					  const char *id,
					  struct deltacloud_storage_snapshot *storage_snapshot);

int deltacloud_create_instance(struct deltacloud_api *api, const char *image_id,
			       const char *name, const char *realm_id,
			       const char *flavor_id,
			       struct deltacloud_instance *inst);
int deltacloud_instance_stop(struct deltacloud_api *api,
			     struct deltacloud_instance *instance);
int deltacloud_instance_reboot(struct deltacloud_api *api,
			       struct deltacloud_instance *instance);
int deltacloud_instance_start(struct deltacloud_api *api,
			      struct deltacloud_instance *instance);

void deltacloud_free(struct deltacloud_api *api);

const char *deltacloud_strerror(int error);

/* Error codes */
#define DELTACLOUD_UNKNOWN_ERROR -1
/* ERROR codes -2, -3, and -4 are reserved for future use */
#define DELTACLOUD_GET_URL_ERROR -5
#define DELTACLOUD_POST_URL_ERROR -6
#define DELTACLOUD_XML_PARSE_ERROR -7
#define DELTACLOUD_URL_DOES_NOT_EXIST -8
#define DELTACLOUD_OOM_ERROR -9
#define DELTACLOUD_INVALID_IMAGE_ERROR -10
#define DELTACLOUD_FIND_ERROR -11

#ifdef __cplusplus
}
#endif

#endif
