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
  i->top = row;
  
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
  i->left = column;
  
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
  i->h = row + 1;
  
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
  i->w = column + 1;
}