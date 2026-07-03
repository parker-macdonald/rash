#ifndef LIB_SLICE_H
#define LIB_SLICE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t length;
  union {
    char *char_ptr;
    uint8_t *u8_ptr;
    void *void_ptr;
  };
} Slice;

#define slice_using_ptr(ptr_, length_) ((Slice){.void_ptr = (ptr_), .length = (length_)})
#define slice_using_cstr(cstr_) slice_using_ptr((cstr_), strlen((cstr_)))
#define slice_using_buffer(buffer_) slice_using_ptr((buffer_).void_ptr, (buffer_).length)

#endif
