#include "lib/buffer.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lib/error.h"
#include "lib/next_pow_2.h"
#include "lib/vector.h"

/*
 * Helper functions to create a new buffer
 */

// constructs a buffer by copying the data at `data` of length `length` bytes.
Buffer buffer_from_ptr(const void *data, size_t length) {
  Buffer buffer;

  buffer._capacity = next_pow_2(length);
  buffer.length = length;
  buffer.data = malloc(buffer._capacity);

  memcpy(buffer.data, data, length);

  return buffer;
}

// constructs a buffer by copying the data at `cstr` of length `strlen(cstr)`.
Buffer buffer_from_cstr(const char *cstr) {
  size_t length = strlen(cstr);
  return buffer_from_ptr(cstr, length);
}

// clone a buffer, creating a new one referencing a copy of the old ones data.
Buffer buffer_clone(const Buffer *other) {
  return buffer_from_ptr(other->data, other->length);
}

// destructor
void buffer_destroy(Buffer *self) {
  VECTOR_DESTROY(*self);
}

/*
 * Helper functions to append something to the end of an existing buffer
 */

void buffer_append_byte(Buffer *self, uint8_t byte) {
  VECTOR_PUSH(*self, byte);
}

void buffer_append_ptr(Buffer *self, const void *data, size_t length) {
  size_t new_length = self->length + length;

  buffer_grow(self, new_length);

  memcpy(self->data + self->length, data, length);

  self->length = new_length;
}

void buffer_append_cstr(Buffer *self, const char *cstr) {
  size_t length = strlen(cstr);
  buffer_append_ptr(self, cstr, length);
}

/*
 * Helper functions to insert into an arbitrary place in an existing buffer
 */

void buffer_insert_ptr(
    Buffer *self, size_t at, const void *data, size_t length
) {
  size_t new_length = self->length + length;

  buffer_grow(self, new_length);

  uint8_t *offset = self->data + at;

  memmove(offset + length, offset, self->length - at);
  memcpy(offset, data, length);

  self->length = new_length;
}

void buffer_insert_byte(Buffer *self, size_t at, uint8_t byte) {
  if (at == self->length) {
    buffer_append_byte(self, byte);
    return;
  }

  buffer_insert_ptr(self, at, &byte, 1);
}

void buffer_insert_cstr(Buffer *self, size_t at, const char *cstr) {
  size_t length = strlen(cstr);
  buffer_insert_ptr(self, at, cstr, length);
}

/*
 * Extra helper functions
 */

// remove n bytes from an arbitrary place in an existing buffer
void buffer_remove_n(Buffer *self, size_t at, size_t count) {
  rash_assert(self->length < at + count, "index out of bounds on remove");

  size_t new_length = self->length - count;

  // removed from the end, no work needs to be done
  if (self->length == at) {
    return;
  }

  memmove(self->data + at, self->data + at + count, self->length - at - count);
  self->length = new_length;
}

// copy the contents of one buffer into another existing buffer
void buffer_copy(Buffer *self, const Buffer *other) {
  buffer_grow(self, other->length);

  memcpy(self->data, other->data, other->length);
  self->length = other->length;
}

// construct a buffer by "slicing" another from index `from` to index `to`.
// `from` is inclusive, `to` is exclusive. i.e. Buffer buffer =
// buffer_from_cstr("hi there"); Buffer slice = buffer_slice(3, buffer.length);
// slice is now "there"
Buffer buffer_slice(const Buffer *self, size_t from, size_t to) {
  return buffer_from_ptr(self->data + from, to - from);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// similar to strcmp, but with buffers
int buffer_compare(const Buffer *self, const Buffer *other) {
 size_t min_length = MIN(self->length, other->length);
	
	  for (size_t i = 0; i < min_length; i++) {
	    int diff = self->data[i] - other->data[i];
	
	    if (diff != 0) {
	      return diff;
	    }
	  }
	
	  if (self->length < other->length) {
	    return -1;
	  }
	  if (self->length > other->length) {
	    return 1;
	  }
	
	  return 0; 
}

void buffer_grow(Buffer *self, size_t grow_to) {
  size_t new_capacity = next_pow_2(grow_to);

  if (new_capacity <= self->_capacity) {
    return;
  }

  self->_capacity = new_capacity;

  void *ptr = realloc(self->data, self->_capacity);

  rash_assert(ptr == NULL, "realloc failed\n");

  self->data = ptr;
}
