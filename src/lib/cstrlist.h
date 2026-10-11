#ifndef CSTRLIST_H
#define CSTRLIST_H

#include "lib/vector.h"

#include <stddef.h>
#include <stdlib.h>

typedef VECTOR(char *) CStrList;

inline void cstr_list_destroy(CStrList *list) {
  for (size_t i = 0; i < list->length; i++) {
    free(list->data[i]);
  }
  VECTOR_DESTROY(*list);
}

#endif
