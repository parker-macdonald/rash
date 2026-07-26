#include "lib/slice.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

void slice_trim_left(Slice *self) {
  while (1) {
    if (self->length == 0) {
      self->void_ptr = NULL;
      break;
    }

    if (!isspace((int)*self->u8_ptr)) {
      break;
    }

    self->u8_ptr++;
    self->length--;
  }
}

void slice_trim_right(Slice *self) {
  while (1) {
    if (self->length == 0) {
      self->void_ptr = NULL;
      break;
    }
    
    if (!isspace((int)self->u8_ptr[self->length - 1])) {
      break;
    }

    self->length--;
  }
}

void slice_trim(Slice *self) {
  slice_trim_left(self);
  slice_trim_right(self);
}
