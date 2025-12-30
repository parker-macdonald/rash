#include "sort.h"

#include <stdlib.h>
#include <string.h>

#include "vec_types.h"

static int compare(const void *a, const void *b) {
  return strcmp(*(const char **)a, *(const char **)b);
}

void sort_strings(strings_t *vec) {
  qsort(vec->data, vec->length, sizeof(char *), compare);
}
