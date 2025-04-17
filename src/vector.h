#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>

#define VECTOR_DEFAULT_SIZE 16

#define VECTOR(type) \
struct { \
  size_t capacity; \
  size_t length; \
  type *data; \
}

#define VECTOR_INIT(vector) \
do { \
  (vector).capacity = VECTOR_DEFAULT_SIZE; \
  (vector).length = 0; \
  (vector).data = malloc(sizeof(*(vector).data) * VECTOR_DEFAULT_SIZE); \
} while(0)

#define VECTOR_PUSH(vector, value) \
do { \
  if ((vector).capacity <= (vector).length) { \
    (vector).capacity *= 2; \
    (vector).data = realloc((vector).data, sizeof(*(vector).data) * (vector).capacity); \
  } \
  (vector).data[(vector).length] = value; \
  (vector).length++; \
} while(0)

#define VECTOR_DESTROY(vector) \
do { \
  free((vector).data); \
  (vector).data = NULL; \
} while(0)

#define VECTOR_CLEAR(vector) \
  (vector).length = 0

#endif
