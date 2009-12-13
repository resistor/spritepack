#ifndef PACK_H
#define PACK_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned* pack_rects(unsigned* r, unsigned num_rects, unsigned width, unsigned* max_y);

#ifdef __cplusplus
}
#endif

#endif
