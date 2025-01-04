#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>

#define VECTOR_DEFAULT_SIZE 16

#define VECTOR(type) \
struct { \
  size_t capacity; \
  size_t index; \
  type *data; \
}

#define VECTOR_INIT(vector) \
{ \
  vector.capacity = VECTOR_DEFAULT_SIZE; \
  vector.index = 0; \
  vector.data = malloc(sizeof(*vector.data) * VECTOR_DEFAULT_SIZE); \
}

#define VECTOR_PUSH(vector, value) \
{ \
  if (vector.capacity <= vector.index) { \
    vector.capacity *= 2; \
    vector.data = realloc(vector.data, vector.capacity); \
  } \
  vector.data[vector.index] = value; \
  vector.index++; \
}

#define VECTOR_DESTROY(vector) \
{ \
  free(vector.data); \
}

#endif
