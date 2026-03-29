#include "lib/next_pow_2.h"

size_t next_pow_2(size_t n) {
  size_t p = 1;
  while (p < n) {
    p *= 2;
  }
  return p;
}
