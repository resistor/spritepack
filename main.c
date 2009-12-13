#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pack.h"
#include "lodepng.h"

typedef struct Img {
    unsigned char* pixels;
    unsigned char* filename;
    unsigned w, h;
} img_t;

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

unsigned int nlpo2(register unsigned int x) {
        if (!(x & (x-1))) return x;
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return(x+1);
}


int main(int argc, char** argv) {
  LodePNG_Decoder decoder;
  LodePNG_Decoder_init(&decoder);
  
  img_t** images = malloc((argc-2) * sizeof(img_t*));
  
  unsigned i  = 0;
  for (i = 0; i < argc-2; ++i) {
    unsigned char *buffer, *image;
    unsigned buffersize, imagesize;
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
  
  qsort(images, argc-2, sizeof(img_t*), image_comparator);


  unsigned* rects = malloc(2 * (argc-2) * sizeof(unsigned));
  for (i = 0; i < argc-2; ++i) {
    rects[2*i] = images[i]->w;
    rects[2*i+1] = images[i]->h;
  }
  
  unsigned max_x = 512;
  unsigned max_y = 513;
  double MP = 0.0;
  unsigned* ret;
  while (max_y > max_x) {
    unsigned new_max_x = max_x * 2;
    unsigned new_max_y;
    unsigned* new_ret = pack_rects(rects, argc-2, new_max_x, &new_max_y);

    if (MP != 0.0 && (double)new_max_x * (double)new_max_y > MP) {
      free(new_ret);
      break;
    }

    max_x = new_max_x;
    max_y = new_max_y;
    MP = (double)max_x * (double)new_max_y;
    ret = new_ret;
  }
  
  /*for (i = 0; i < argc-2; ++i) {
    printf("file %s @ X: %d, Y: %d\n",
           images[i]->filename, ret[2*i], ret[2*i+1]);
  }*/
  
  unsigned char* out_image = calloc(4 * max_x * max_y, sizeof(unsigned char));
  
  for (i = 0; i < argc-2; ++i) {
    unsigned off_x = ret[2*i];
    unsigned off_y = max_y - ret[2*i+1] - images[i]->h;
    
    unsigned x, y;
    for (x = 0; x < images[i]->w; ++x) {
      for (y = 0; y < images[i]->h; ++y) {
        unsigned char r_in = images[i]->pixels[4 * y * images[i]->w + 4 * x + 0];
        unsigned char g_in = images[i]->pixels[4 * y * images[i]->w + 4 * x + 1];
        unsigned char b_in = images[i]->pixels[4 * y * images[i]->w + 4 * x + 2];
        unsigned char a_in = images[i]->pixels[4 * y * images[i]->w + 4 * x + 3];
        
        unsigned x_pix = off_x + x;
        unsigned y_pix = off_y + y;
        
        out_image[4 * y_pix * max_x + 4 * x_pix + 0] = r_in;
        out_image[4 * y_pix * max_x + 4 * x_pix + 1] = g_in;
        out_image[4 * y_pix * max_x + 4 * x_pix + 2] = b_in;
        out_image[4 * y_pix * max_x + 4 * x_pix + 3] = a_in;
      }
    }
  }
  
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
