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
void copy_image(struct image *dst, struct image *src);
void print_image(struct image *image, FILE *stream);
void print_image_list(struct image **images, FILE *stream);
void free_image(struct image *image);
void free_image_list(struct image **images);

#endif
