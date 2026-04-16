#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>

#include "vector.h"

typedef VECTOR(uint8_t) Buffer;

void buffer_append_string(Buffer *self, const char *str);

void buffer_copy(Buffer *dest, const Buffer *src);

void buffer_insert(Buffer *buffer, size_t buffer_offset, uint8_t byte);

void buffer_insert_bulk(
    Buffer *buffer, const uint8_t *src, size_t src_len, size_t buffer_offset
);

void buffer_remove_bulk(Buffer *buffer, size_t offset, size_t count);

// constructs a buffer by copying the data at `data` of length `length`. if this
// were rust i would say that the returned buffer owns it's data.
Buffer buffer_from(const uint8_t *data, size_t length);

// constructs a buffer using the pointer `data` and the length `length`. if this
// were rust i would say the the returned buffer borrows it's data.
Buffer buffer_using(const uint8_t *data, size_t length);

#endif
