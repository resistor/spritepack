#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pack.h"
#include "lodepng.h"

/* img_t - A simple wrapper for image data, filename, and width/height */
typedef struct Img {
    unsigned char* pixels;
    char* filename;
    unsigned w, h;
} img_t;

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

/* Usage: spritepack outfile infile1 infile2 ... */
int main(int argc, char** argv) {
  /* Init the PNG decoder */
  LodePNG_Decoder decoder;
  LodePNG_Decoder_init(&decoder);
  
  /* Buffer to hold img_t*'s for the input images */
  img_t** images = malloc((argc-2) * sizeof(img_t*));
  
  /* Loop over the program arguments, decoding input PNGs and
   * creating img_t*'s for them */
  int i;
  for (i = 0; i < argc-2; ++i) {
    unsigned char *buffer, *image;
    size_t buffersize, imagesize;
    LodePNG_loadFile(&buffer, &buffersize, argv[i+2]);
    LodePNG_decode(&decoder, &image, &imagesize, buffer, buffersize);
    
    images[i] = malloc(sizeof(img_t));
    images[i]->pixels = image;
    images[i]->w = decoder.infoPng.width;
    images[i]->h = decoder.infoPng.height;
    images[i]->filename = argv[i+2];
    free(buffer);
  }
  
  LodePNG_Decoder_cleanup(&decoder);
  
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
  
  /* Spew input image offsets */
  for (i = 0; i < argc-2; ++i) {
    printf("file %s @ X: %d, Y: %d\n",
           images[i]->filename, ret[2*i], ret[2*i+1]);
  }
  
  /* Allocate the output image */
  unsigned char* out_image = calloc(4 * max_x * max_y, sizeof(unsigned char));
  
  /* Blit each input image into the output image at the offset determined
   * by the packing.  This is a bit complicated, since the decoded pixels
   * are indexed from the top-left, while the packing uses bottom-left indexing */
  for (i = 0; i < argc-2; ++i) {
    unsigned off_x = ret[2*i];
    unsigned off_y = max_y - ret[2*i+1] - images[i]->h;
    
    unsigned y;
      for (y = 0; y < images[i]->h; ++y) {
        unsigned y_pix = off_y + y;
        memcpy(out_image + (4 * y_pix * max_x + 4 * off_x),
               images[i]->pixels + (4 * y * images[i]->w),
               4 * images[i]->w);
      }
  }
  
  /* Encode and output the output image */
  unsigned char* buffer;
  size_t buffersize;
  LodePNG_Encoder encoder;
  LodePNG_Encoder_init(&encoder);
  LodePNG_encode(&encoder, &buffer, &buffersize, out_image, max_x, max_y);
  LodePNG_saveFile(buffer, buffersize, argv[1]);
  LodePNG_Encoder_cleanup(&encoder);
  free(buffer);
  free(out_image);

  return 0;
}
