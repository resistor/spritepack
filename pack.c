#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "pack.h"

typedef struct Edge {
  struct Edge *prev;
  struct Edge *next;
  unsigned left_x, right_x, height;
} edge_t;

edge_t* list;
edge_t** heap;
unsigned heap_size;
unsigned heap_max;

static void init(unsigned w) {
  list = malloc(sizeof(edge_t));
  list->next = NULL;
  list->prev = NULL;
  list->height = 0;
  list->left_x = 0;
  list->right_x = w - 1;
  
  heap = malloc(4 * sizeof(edge_t*));
  heap[0] = list;
  heap_size = 1;
  heap_max = 4;
}

static void deinit(void) {
  while (list) {
    edge_t* tmp = list;
    list = list->next;
    free(tmp);
  }
  
  free(heap);
}

static void insert_heap(edge_t* e) {
  ++heap_size;
  if (heap_size == heap_max) {
    heap_max *= 2;
    heap = realloc(heap, heap_max * sizeof(edge_t*));
  }
  
  unsigned idx = heap_size-1;
  heap[idx] = e;
  
  while (idx > 0 && heap[(idx-1)/2]->height > e->height) {
    heap[idx] = heap[(idx-1)/2];
    heap[(idx-1)/2] = e;
    idx = (idx-1)/2;
  }
}

static edge_t* lowest_edge(void) {
  return heap[0];
}

static void update_heap() {
  unsigned idx = 0;
  unsigned left_idx = 2*idx+1;
  unsigned right_idx = 2*idx+2;
  
  while ((left_idx < heap_size && heap[idx]->height > heap[left_idx]->height) ||
       (right_idx < heap_size && heap[idx]->height > heap[right_idx]->height)) {
    unsigned low_idx = left_idx;
    if (right_idx < heap_size &&
        heap[right_idx]->height < heap[low_idx]->height)
      low_idx = right_idx;
    
    edge_t* tmp = heap[idx];
    heap[idx] = heap[low_idx];
    heap[low_idx] = tmp;
    
    idx = low_idx;
    left_idx = 2*idx+1;
    right_idx = 2*idx+2;
  }
}

static void pop_heap() {
  heap[0] = heap[heap_size-1];
  --heap_size;
  update_heap();
}

// When we've determined that this edge is too narrow for
// any rectangle to fit on, we raise it to match its next
// tallest neighbor, and merge the two into one larger edge.
static void raise_edge(edge_t* e) {
  pop_heap();

  edge_t* left = e->prev;
  edge_t* right = e->next;
  
  if (!right) {
    left->right_x = e->right_x;
    left->next = NULL;
  } else if (!left) {
    right->left_x = e->left_x;
    right->prev = NULL;
  } else {
    if (right->height > left->height)
      left->right_x = e->right_x;
    else
      right->left_x = e->left_x;
    
    left->next = right;
    right->prev = left;
  }
  
  if (list == e) list = e->next;
  
  free(e);
}

// Given an edge and the width of the rectangle we're placing onto it,
// split the edge into the part taken up by the rectangle, and the part
// left over.
static void split_edge(edge_t* e, unsigned w) {
  // Use the left-placement strategy, from simplicity
  edge_t* new_edge = malloc(sizeof(edge_t));
  new_edge->prev = e;
  new_edge->next = e->next;
  e->next = new_edge;
  if (new_edge->next)
    new_edge->next->prev = new_edge;
  
  new_edge->right_x = e->right_x;
  e->right_x = e->left_x + w - 1;
  new_edge->left_x = e->right_x + 1;
  new_edge->height = e->height;
  
  update_heap();
  insert_heap(new_edge);
}

// rect_t - Linked list node for rectangles
typedef struct Rect {
  struct Rect* prev;
  struct Rect* next;
  unsigned w, h, idx;
} rect_t;

// rect2list - Produce a linked list of rect_t*'s from an array
// of pairs of unsigneds.
rect_t* rects2list(unsigned* rects, unsigned num_rects) {
  rect_t* head = (rect_t*)malloc(sizeof(rect_t));
  rect_t* ret = head;
  head->next = 0;
  head->prev = 0;
  head->w = rects[0];
  head->h = rects[1];
  head->idx = 0;
  
  unsigned i;
  for (i = 1; i < num_rects; ++i) {
    rect_t* node = (rect_t*)malloc(sizeof(rect_t));
    node->prev = head;
    node->next = 0;
    node->idx = i;
    head->next = node;
    node->w = rects[2*i];
    node->h = rects[2*i+1];
    head = node;
  }
  
  return ret;
}

unsigned* pack_rects(unsigned* r, unsigned num_rects, unsigned width, unsigned* max_y) {
  init(width);
  
  rect_t* rect_list = rects2list(r, num_rects);
  
  unsigned* results = malloc(num_rects * 2 * sizeof(unsigned));
  
  unsigned i;
  for (i = 0; i < num_rects; ++i) {
    // Find the edge on which to place the rect
    edge_t* lowest = lowest_edge();
    while (lowest->right_x - lowest->left_x + 1 < rect_list->w) {
      raise_edge(lowest);
      lowest = lowest_edge();
    }
    
    // Find the rect to place on the edge
    rect_t* widest = rect_list;
    rect_t* curr = rect_list->next;
    while (curr) {
      if (curr->w > lowest->right_x - lowest->left_x + 1) break;
      widest = curr;
      curr = curr->next;
    }
    
    // Unlink the rect we're placing
    if (widest->prev)
      widest->prev->next = widest->next;
    else
        rect_list = widest->next;
    if (widest->next)
      widest->next->prev = widest->prev;
    
    // If there's space left over on the edge, split it
    if (lowest->right_x - lowest->left_x > widest->w)
      split_edge(lowest, widest->w);
  
    // Record the placement
    results[2*widest->idx] = lowest->left_x;
    results[2*widest->idx+1] = lowest->height;
  
    lowest->height += widest->h;
    update_heap();
    
    // Delete the rect
    free(widest);
  }
  
  edge_t* highest = list;
  edge_t* curr = list->next;
  while (curr) {
    if (curr->height > highest->height) highest = curr;
    curr = curr->next;
  }
  
  *max_y = highest->height;
  
  deinit();
  
  return results;
}
