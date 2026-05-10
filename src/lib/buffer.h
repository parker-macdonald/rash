#ifndef BUFFER_H
#define BUFFER_H

#include "lib/vector.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// a buffer will always own it's data
typedef struct {
  uint8_t *data;
  size_t length;
  size_t _capacity;
} Buffer;

// a span will never own it's data
typedef struct {
  const uint8_t *data;
  size_t length;
} Span;

typedef VECTOR(Buffer) BufferList;

/*
 * Helper functions to create a new buffer
 */

// constructs a buffer by copying the data at `data` of length `length` bytes.
Buffer buffer_from_ptr(const void *data, size_t length);

// constructs a buffer by copying the data at `cstr` of length `strlen(cstr)`.
Buffer buffer_from_cstr(const char *cstr);

// construct a buffer by copying the data from the span
Buffer buffer_from_span(const Span *span);

// clone a buffer, creating a new one referencing a copy of the old ones data.
Buffer buffer_clone(const Buffer *other);

// destructor
void buffer_destroy(Buffer *self);

/*
 * Helper functions to append something to the end of an existing buffer
 */

void buffer_append_byte(Buffer *self, uint8_t byte);

void buffer_append_cstr(Buffer *self, const char *cstr);

void buffer_append_span(Buffer *self, const Span *span);

void buffer_append_ptr(Buffer *self, const void *data, size_t length);

/*
 * Helper functions to insert into an arbitrary place in an existing buffer
 */

void buffer_insert_byte(Buffer *self, size_t at, uint8_t byte);

void buffer_insert_cstr(Buffer *self, size_t at, const char *cstr);

void buffer_insert_span(Buffer *self, size_t at, const Span *span);

void buffer_insert_ptr(Buffer *self, size_t at, const void *data,
                       size_t length);

/*
 * Other useful functions for dealing with buffers
 */

// remove n bytes from an arbitrary place in an existing buffer
void buffer_remove_n(Buffer *buffer, size_t at, size_t count);

// copy the contents of one buffer into another existing buffer
void buffer_copy(Buffer *dest, const Buffer *src);

void buffer_replace(Buffer *self, uint8_t search_for, uint8_t replace_with);

// return a pointer to the buffers data ended with a null terminator
// this pointer is invalidated the next time the buffer is modified
const char *buffer_cstr(Buffer *self);

// construct a buffer by "slicing" another from index from to index to. from is
// inclusive, to is exclusive.
// i.e.
// Buffer buffer = buffer_from_cstr("hi there");
// Buffer slice = buffer_slice(3, buffer.length);
// slice is now "there"
Span buffer_slice(const Buffer *self, size_t from, size_t to);

/*
 * Helper functions to create a new slice
 */

// constructs a span using the data at `data` of length `length`.
Span span_from_ptr(const void *data, size_t length);

// constructs a span using the data at `cstr` of length `strlen(cstr)`.
Span span_from_cstr(const char *cstr);

// constructs a span using the data in `buffer`.
Span span_from_buffer(const Buffer *buffer);

// check whether a span contains a certain byte
bool span_contains(const Span *self, uint8_t search_for);

size_t span_index_of(const Span *self, uint8_t search_for);

size_t span_last_index_of(const Span *self, uint8_t search_for);

// similar to strcmp, but with spans
int span_compare(const Span *s1, const Span *s2);

#endif
