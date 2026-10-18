#include "lib/cstrlist.h"

#include <stddef.h>
#include <stdlib.h>

void cstr_list_destroy(CStrList *list) {
  for (size_t i = 0; i < list->length; i++) {
    free(list->data[i]);
  }
  VECTOR_DESTROY(*list);
}
