#ifndef IMAGES_H
#define IMAGES_H

struct image {
  char *href;
  char *id;
  char *description;
  char *architecture;
  char *owner_id;

  struct image *next;
};

int add_to_image_list(struct image **images, char *href, char *id,
		      char *description, char *architecture, char *owner_id);
void print_images_list(struct image **images, FILE *stream);
void free_images_list(struct image **images);

#endif
