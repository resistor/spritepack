#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>

#include "img.h"
#include "pack.h"

/* image_comparator - a qsort-compatible comparison function for img_t*'s
 * Sorts first by width, then height */
int image_comparator(const void* a, const void* b) {
  img_t* aimg = *(img_t**)a;
  img_t* bimg = *(img_t**)b;
  
  if (aimg->w < bimg->w)
    return -1;
  else if (aimg->w > bimg->w)
    return 1;
  else {
    if (aimg->h < bimg->h)
      return -1;
    if (aimg->h > bimg->h)
      return 1;
    else
      return 0;
  }
}

int filename_comparator(const void* a, const void* b) {
  img_t* aimg = *(img_t**)a;
  img_t* bimg = *(img_t**)b;
  
  return strcmp(aimg->filename, bimg->filename);
}

img_t* load_png(char* filename) {
  FILE* fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "ERROR: Could not open input file %s\n", filename);
    exit(-1);
  }
  
  png_byte pngsig[8];
  fread(pngsig, 1, 8, fp);
  int is_png = !png_sig_cmp(pngsig, 0, 8);
  if (!is_png) {
    fprintf(stderr, "ERROR: File %s is not a PNG!\n", filename);
    fclose(fp);
    exit(-1);
  }
  
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if (!png_ptr) {
    fprintf(stderr, "ERROR: Could not open input file %s\n", filename);
    fclose(fp);
    exit(-1);
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, 0, 0);
    fprintf(stderr, "ERROR: Could not open input file %s\n", filename);
    fclose(fp);
    exit(-1);
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    fprintf(stderr, "ERROR: Could not open input file %s\n", filename);
    fclose(fp);
    exit(-1);
  }
  
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fprintf(stderr, "ERROR: Could not open input file %s\n", filename);
    fclose(fp);
    exit(-1);
  }
  
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  
  png_uint_32 width, height;
  int bit_depth, color_type;
  
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height,
               &bit_depth, &color_type, NULL, NULL, NULL);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_expand (png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand (png_ptr);
  if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_expand (png_ptr);
  if (bit_depth == 16)
    png_set_strip_16 (png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY ||
    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb (png_ptr);

  png_read_update_info (png_ptr, info_ptr);
  png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
    NULL, NULL, NULL);

  unsigned row_bytes = png_get_rowbytes (png_ptr, info_ptr);

  unsigned char* png_pixels = malloc (row_bytes * height * sizeof (png_byte));
  unsigned char** ret = malloc(height * sizeof (png_bytep));
  unsigned i;
  for (i = 0; i < (height); i++)
    ret[i] = png_pixels + i * row_bytes;

  png_read_image (png_ptr, ret);
  png_read_end (png_ptr, info_ptr);
  fclose(fp);
  
  img_t* image = malloc(sizeof(img_t));
  image->pixels = ret;
  image->top = 0;
  image->left = 0;
  image->w = width;
  image->h = height;
  image->center_x = width / 2;
  image->center_y = height / 2;
  image->filename = filename;

  return image;
}

void write_png(char* filename, unsigned w, unsigned h,
               unsigned char** data, png_text* comments) {
  FILE* fp = fopen(filename, "wb");
  if (!fp) {
    fprintf(stderr, "ERROR: Could not open output file %s\n", filename);
    exit(-1);
  }
  
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
   if (!png_ptr) {
     fprintf(stderr, "ERROR: Could not open output file %s\n", filename);
     fclose(fp);
     exit(-1);
   }

   png_infop info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) {
     png_destroy_read_struct(&png_ptr, 0, 0);
     fprintf(stderr, "ERROR: Could not open output file %s\n", filename);
     fclose(fp);
     exit(-1);
   }
   
   if (setjmp(png_jmpbuf(png_ptr))) {
     png_destroy_write_struct(&png_ptr, &info_ptr);
     fprintf(stderr, "ERROR: Could not open output file %s\n", filename);
     fclose(fp);
     exit(-1);
   }
   
   png_init_io(png_ptr, fp);
   png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB_ALPHA,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);
  png_set_text(png_ptr, info_ptr, comments, 1);
  
   png_set_rows(png_ptr, info_ptr, data);
   png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
   
   fclose(fp);
}

/* Usage: spritepack outfile infile1 infile2 ... */
int main(int argc, char** argv) {
  /* Buffer to hold img_t*'s for the input images */
  img_t** images = malloc((argc-2) * sizeof(img_t*));
  
  /* Loop over the program arguments, decoding input PNGs and
   * creating img_t*'s for them */
  int i;
  for (i = 0; i < argc-2; ++i) {
    images[i] = load_png(argv[i+2]);
    autotrim(images[i]);
  }
  
  /* Sort the images, because pack_rects expects input in sorted order */
  qsort(images, argc-2, sizeof(img_t*), image_comparator);

  /* Create a proxy array of rectangles, which pack_rects will use as input.
   * Note that the indexing matches the indexing into the images array for
   * easy correspondence. */
  unsigned biggest_width = 0;
  unsigned width_sum = 0;
  unsigned* rects = malloc(2 * (argc-2) * sizeof(unsigned));
  for (i = 0; i < argc-2; ++i) {
    rects[2*i] = images[i]->w;
    rects[2*i+1] = images[i]->h;
    if (images[i]->w > biggest_width) biggest_width = images[i]->w;
    width_sum += images[i]->w;
  }
  
  if (width_sum > 2048) width_sum = 2048;
  
  /* Explore all valid widths to find the most efficient packing. */
  unsigned best_MP = 0, max_x, max_y;
  unsigned* ret;
  unsigned curr_width;
  for (curr_width = biggest_width; curr_width <= width_sum; ++curr_width) {
    unsigned curr_height;
    unsigned* new_ret = pack_rects(rects, argc-2, curr_width, &curr_height);

    if (curr_height > 2048) {
      free(new_ret);
      continue;
    }

    if (best_MP == 0 || curr_width * curr_height < best_MP) {
      max_x = curr_width;
      max_y = curr_height;
      ret = new_ret;
      best_MP = curr_width * curr_height;
    } else
      free(new_ret);
  }
  
  // Copy the data returned by the packer back into the img_t structures.
  for (i = 0; i < argc-2; ++i) {
    images[i]->offset_x = ret[2*i];
    images[i]->offset_y = max_y - ret[2*i+1] - images[i]->h;
  }
  free(ret);
  
  // Sort the images by filename again
  qsort(images, argc-2, sizeof(img_t*), filename_comparator);
  
  /* Spew input image offsets */
  png_text comments;
  comments.compression = PNG_TEXT_COMPRESSION_zTXt;
  comments.key = "sprite";
  
  
  // Setup the comments string for printf'ing.
  unsigned text_space = 2 * (argc-1) + 1;
  comments.text = malloc(text_space);
  comments.text[0] = 0;
  
  char* num_sprites;
  asprintf(&num_sprites, "%d", argc-2);
  while (strlen(comments.text) + strlen(num_sprites) + 1 > text_space) {
    text_space *= 2;
    comments.text = realloc(comments.text, text_space);
  }
  comments.text = strncat(comments.text, num_sprites, strlen(num_sprites));
  
  for (i = 0; i < argc-2; ++i) {
    char* output;
    asprintf(&output, ", %d, %d, %d, %d, %d, %d",
/* top left corner */     images[i]->offset_x, images[i]->offset_y,
/* bottom right corner */ images[i]->offset_x + images[i]->w, images[i]->offset_y + images[i]->h,
/* center offset */       images[i]->center_x, images[i]->center_y);
    //printf("%s\n", output);
    
    while (strlen(comments.text) + strlen(output) + 1 > text_space) {
      text_space *= 2;
      comments.text = realloc(comments.text, text_space);
    }
    comments.text = strncat(comments.text, output, strlen(output));
  }
  
  comments.text_length = strlen(comments.text);
  
  /* Allocate the output image */
  unsigned char** out_image = malloc(max_y * sizeof(unsigned char*));
  for (i = 0; i < (int)max_y; ++i)
    out_image[i] = calloc(4 * max_x, sizeof(unsigned char));
  
  /* Blit each input image into the output image at the offset determined
   * by the packing.  This is a bit complicated, since the decoded pixels
   * are indexed from the top-left, while the packing uses bottom-left indexing */
  for (i = 0; i < argc-2; ++i) {
    unsigned off_x = images[i]->offset_x;
    unsigned off_y = images[i]->offset_y;
    
    unsigned y;
    for (y = 0; y < images[i]->h; ++y) {
      unsigned y_pix = off_y + y;
      memcpy(out_image[y_pix] + 4 * off_x,
             images[i]->pixels[images[i]->top + y] + 4 * images[i]->left,
             4 * images[i]->w);
    }
  }
  
  /* Encode and output the output image */
  write_png(argv[1], max_x, max_y, out_image, &comments);

  return 0;
}
