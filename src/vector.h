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
{ \
  (vector).capacity = VECTOR_DEFAULT_SIZE; \
  (vector).length = 0; \
  (vector).data = malloc(sizeof(*(vector).data) * VECTOR_DEFAULT_SIZE); \
}

#define VECTOR_PUSH(vector, value) \
{ \
  if ((vector).capacity <= (vector).length) { \
    (vector).capacity *= 2; \
    (vector).data = realloc((vector).data, sizeof(*(vector).data) * (vector).capacity); \
  } \
  (vector).data[(vector).length] = value; \
  (vector).length++; \
}

#define VECTOR_DESTROY(vector) \
{ \
  free((vector).data); \
  (vector).data = NULL; \
}

#define VECTOR_CLEAR(vector) \
  (vector).length = 0

#endif
