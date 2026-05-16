#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>

#include "vector.h"

typedef VECTOR(uint8_t) Buffer;

/*
 * Helper functions to create a new buffer
 */

// constructs a buffer by copying the data at `data` of length `length` bytes.
Buffer buffer_from_ptr(const void *data, size_t length);

// constructs a buffer by copying the data at `cstr` of length `strlen(cstr)`.
Buffer buffer_from_cstr(const char *cstr);

// clone a buffer, creating a new one referencing a copy of the old ones data.
Buffer buffer_clone(const Buffer *other);

// destructor
void buffer_destroy(Buffer *self);

/*
 * Helper functions to append something to the end of an existing buffer
 */

void buffer_append_byte(Buffer *self, uint8_t byte);

void buffer_append_cstr(Buffer *self, const char *cstr);

void buffer_append_ptr(Buffer *self, const void *data, size_t length);

/*
 * Helper functions to insert into an arbitrary place in an existing buffer
 */

void buffer_insert_byte(Buffer *self, size_t at, uint8_t byte);

void buffer_insert_cstr(Buffer *self, size_t at, const char *cstr);

void buffer_insert_ptr(
    Buffer *self, size_t at, const void *data, size_t length
);

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

// resize the buffer to hold at least `grow_to` bytes. nothing is done if the
// buffer can already hold `grow_to` bytes
void buffer_grow(Buffer *self, size_t grow_to);

#endif
