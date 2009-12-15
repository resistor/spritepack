#ifndef IMG_H
#define IMG_H

/* img_t - A simple wrapper for image data, filename, and width/height */
typedef struct Img {
    unsigned char** pixels;
    char* filename;
    unsigned w, h;
} img_t;

#endif
