#ifndef IMAGE_H
#define IMAGE_H

struct image {
  char *href;
  char *id;
  char *description;
  char *architecture;
  char *owner_id;
  char *name;

  struct image *next;
};

int add_to_image_list(struct image **images, const char *href, const char *id,
		      const char *description, const char *architecture,
		      const char *owner_id, const char *name);
void print_image_list(struct image **images, FILE *stream);
void free_image_list(struct image **images);

#endif
