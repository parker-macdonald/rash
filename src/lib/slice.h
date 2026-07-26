#ifndef LIB_SLICE_H
#define LIB_SLICE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t length;
  union {
    const char *char_ptr;
    const uint8_t *u8_ptr;
    const void *void_ptr;
  };
} Slice;

#define slice_lit_from_ptr(ptr_, length_) ((Slice){.void_ptr = (ptr_), .length = (length_)})
#define slice_lit_from_cstr(cstr_) slice_lit_from_ptr((cstr_), strlen((cstr_)))
#define slice_lit_from_literal(cstr_) slice_lit_from_ptr((cstr_), sizeof(cstr_) - 1)
#define slice_lit_from_buffer(buffer_) slice_lit_from_ptr((buffer_)->void_ptr, (buffer_)->length)

void slice_trim_left(Slice *self);

void slice_trim_right(Slice *self);

void slice_trim(Slice *self);

#endif
