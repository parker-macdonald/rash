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

#define slice_from_ptr(ptr_, length_) ((Slice){.void_ptr = (ptr_), .length = (length_)})
#define slice_from_literal(cstr_) slice_from_ptr((cstr_), sizeof(cstr_) - 1)
#define slice_from_buffer(buffer_) slice_from_ptr((buffer_)->void_ptr, (buffer_)->length)

// this is not a macro cause it calls strlen
Slice slice_from_cstr(const char *cstr);

void slice_trim_left(Slice *self);

void slice_trim_right(Slice *self);

void slice_trim(Slice *self);

#endif
