#ifndef BUFFER_H
#define BUFFER_H

#include "lib/attrib.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t _capacity;
  size_t length;
  union {
    uint8_t *u8_ptr;
    char *char_ptr;
    void *void_ptr;
  };
} Buffer;

/*
 * Helper functions to create a new buffer
 */

// construct a buffer with length 0 and capacty of `next_pow_2(capacity)`
Buffer buffer_create(size_t capacity);

// constructs a buffer by copying the data at `data` of length `length` bytes.
Buffer buffer_from_ptr(const void *data, size_t length);

// constructs a buffer by copying the data at `cstr` of length `strlen(cstr)`.
Buffer buffer_from_cstr(const char *cstr);

// construct a buffer with the results of a call to sprintf
ATTRIB_PRINTF(1, 2)
Buffer buffer_from_format(const char *format, ...);

// clone a buffer, creating a new one referencing a copy of the old ones data.
Buffer buffer_clone(const Buffer *other);

// destructor
void buffer_destroy(Buffer *self);

/*
 * Helper functions to append something to the end of an existing buffer
 */

void buffer_append_byte(Buffer *self, uint8_t byte);

void buffer_append_char(Buffer *self, char character);

void buffer_append_cstr(Buffer *self, const char *cstr);

void buffer_append_ptr(Buffer *self, const void *data, size_t length);

void buffer_append_buffer(Buffer *self, const Buffer *other);

/*
 * Helper functions to insert into an arbitrary place in an existing buffer
 */

void buffer_insert_byte(Buffer *self, size_t at, uint8_t byte);

void buffer_insert_char(Buffer *self, size_t at, char character);

void buffer_insert_cstr(Buffer *self, size_t at, const char *cstr);

void buffer_insert_ptr(Buffer *self, size_t at, const void *data,
                       size_t length);

void buffer_insert_buffer(Buffer *self, size_t at, const Buffer *other);

/*
 * Extra helper functions
 */

// remove n bytes from an arbitrary place in an existing buffer
void buffer_remove_n(Buffer *self, size_t at, size_t count);

// copy the contents of one buffer into another existing buffer
void buffer_copy(Buffer *self, const Buffer *other);

// construct a buffer by "slicing" another from index `from` to index `to`.
// `from` is inclusive, `to` is exclusive. i.e. Buffer buffer =
// buffer_from_cstr("hi there"); Buffer slice = buffer_slice(3, buffer.length);
// slice is now "there"
Buffer buffer_slice(const Buffer *self, size_t from, size_t to);

// similar to strcmp, but with buffers
int buffer_compare(const Buffer *self, const Buffer *other);

// compare a buffer to a cstr
int buffer_compare_cstr(const Buffer *self, const char *cstr);

// resize the buffer to hold at least `grow_to` bytes. nothing is done if the
// buffer can already hold `grow_to` bytes
void buffer_grow_to(Buffer *self, size_t grow_to);

// resize the buffer to hold at least `self->length + grow_by` bytes. nothing is
// done if the buffer can already hold `self->length + grow_by` bytes
void buffer_grow_by(Buffer *self, size_t grow_by);

// modifies `self` by adding a null terminator but the length remains unchanged.
// this returns a pointer to the buffer's data casted to a `char *`
char *buffer_cstr(Buffer *self);

// set the length of a buffer to zero
void buffer_clear(Buffer *self);

size_t buffer_find_from_offset(const Buffer *self, uint8_t search_for, size_t start_from);

#endif
