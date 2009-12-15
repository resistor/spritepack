#include "img.h"

void autotrim(img_t* i) {
  unsigned row = 0;
  unsigned column = 0;
  
  // Trim top
  for (row = 0; row < i->h; ++row) {
    unsigned char isBlank = 1;
    for (column = 0; column < i->w; ++column)
      if (i->pixels[row][4*column + 3]) {
        isBlank = 0;
        break;
      }
    
    if (!isBlank)
      break;
  }
  unsigned top_most = row;
  
  // Trim left
  for (column = 0; column < i->w; ++column) {
    unsigned char isBlank = 1;
    for (row = 0; row < i->h; ++row)
      if (i->pixels[row][4*column + 3]) {
        isBlank = 0;
        break;
      }
    
    if (!isBlank)
      break;
  }
  unsigned left_most = column;
  
  // Trip bottom
  for (row = i->h - 1; row != ~0U; --row) {
    unsigned char isBlank = 1;
    for (column = 0; column < i->w; ++column)
      if (i->pixels[row][4*column + 3]) {
        isBlank = 0;
        break;
      }
    
    if (!isBlank)
      break;
  }
  unsigned bottom_most = row;
  
  // Trip right
  for (column = i->w - 1; column != ~0U; --column) {
    unsigned char isBlank = 1;
    for (row = 0; row < i->h; ++row)
      if (i->pixels[row][4*column + 3]) {
        isBlank = 0;
        break;
      }
    
    if (!isBlank)
      break;
  }
  unsigned right_most = column;
  
  i->top = top_most;
  i->left = left_most;
  i->h = bottom_most - top_most + 1;
  i->w = right_most - left_most + 1;
}