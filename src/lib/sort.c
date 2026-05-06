#include "sort.h"

#include <stdlib.h>

#include "lib/string.h"

static int compare(const void *a, const void *b) {
  return string_compare((const String *)a, (const String *)b);
}

void sort_strings(StringList *vec) {
  qsort(vec->data, vec->length, sizeof(String), compare);
}
