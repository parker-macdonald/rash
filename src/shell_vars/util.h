#ifndef SHELL_VARS_UTIL_H
#define SHELL_VARS_UTIL_H

#include <stdbool.h>
#include <stdint.h>

// if c is alpha numeric or an underscore
bool is_ident(uint8_t c);

// if c is a letter or an underscore
bool is_begin_ident(uint8_t c);

// if cstr is a valid identifier
bool is_ident_cstr(const char *cstr);

#endif
