#include "util.h"

#include <stdbool.h>
#include <stdint.h>

// if c is alpha numeric or an underscore
bool is_ident(uint8_t c) {
  return
    (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') ||
    (c >= '0' && c <= '9') ||
    c == '_';
}

// if c is a letter or an underscore
bool is_begin_ident(uint8_t c) {
  return
    (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') ||
    c == '_';
}

// if cstr is a valid identifier
bool is_ident_cstr(const char *cstr) {
  if (!is_begin_ident((uint8_t)*cstr)) {
    return false;
  }

  cstr++;

  while (*cstr != '\0') {
    if (!is_ident((uint8_t)*cstr)) {
      return false;
    }

    cstr++;
  }

  return true;
}
