#include "sort.h"

#include <stdlib.h>
#include <string.h>

#include "lib/string.h"

static int compare(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b);
}

void sort_strings(StringList *vec) {
  qsort(vec->data, vec->length, sizeof(char *), compare);
}
