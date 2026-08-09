#ifndef BUFFER_H
#define BUFFER_H

#include "lib/attrib.h"
#include "lib/vector.h"

#include <stdbool.h>
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

#define buffer_append(self, to_append) \
  _Generic( \
    to_append, \
    char *:         buffer_append_cstr, \
    const char *:   buffer_append_cstr, \
    Buffer *:       buffer_append_buffer, \
    const Buffer *: buffer_append_buffer, \
    int:            buffer_append_byte, \
    uint8_t:        buffer_append_byte, \
    char:           buffer_append_char \
  ) (self, to_append)

/*
 * Helper functions to insert into an arbitrary place in an existing buffer
 */

void buffer_insert_byte(Buffer *self, size_t at, uint8_t byte);

void buffer_insert_char(Buffer *self, size_t at, char character);

void buffer_insert_cstr(Buffer *self, size_t at, const char *cstr);

void buffer_insert_ptr(Buffer *self, size_t at, const void *data,
                       size_t length);

void buffer_insert_buffer(Buffer *self, size_t at, const Buffer *other);

#define buffer_insert(self, at, to_insert) \
  _Generic( \
    to_insert, \
    char *:         buffer_insert_cstr, \
    const char *:   buffer_insert_cstr, \
    Buffer *:       buffer_insert_buffer, \
    const Buffer *: buffer_insert_buffer, \
    int:            buffer_insert_byte, \
    uint8_t:        buffer_insert_byte, \
    char:           buffer_insert_char \
  ) (self, at, to_insert)

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

size_t buffer_find_next(const Buffer *self, uint8_t search_for, size_t start_from);

size_t buffer_find_prev(const Buffer *self, uint8_t search_for, size_t start_from);

#define buffer_find_first(self, search_for) buffer_find_next(self, search_for, 0)

#define buffer_find_last(self, search_for) buffer_find_prev(self, search_for, (self)->length)

bool buffer_contains_byte(const Buffer *self, uint8_t search_for);

// functions to check if a buffer starts with something

bool buffer_starts_with_ptr(const Buffer *self, const void *starts_with, size_t starts_with_length);

bool buffer_starts_with_cstr(const Buffer *self, const char *starts_with);

bool buffer_starts_with_buffer(const Buffer *self, const Buffer *starts_with);

bool buffer_starts_with_char(const Buffer *self, char starts_with);

bool buffer_starts_with_byte(const Buffer *self, uint8_t starts_with);

#define buffer_starts_with(self, starts_with) \
  _Generic( \
    starts_with, \
    char *:         buffer_starts_with_cstr, \
    const char *:   buffer_starts_with_cstr, \
    Buffer *:       buffer_starts_with_buffer, \
    const Buffer *: buffer_starts_with_buffer, \
    int:            buffer_starts_with_byte, \
    uint8_t:        buffer_starts_with_byte, \
    char:           buffer_starts_with_char \
  ) (self, starts_with)

// ------ buffer list -------

// since this is just a typedef'd vector, you can use the vector macros on it (like VECTOR_PUSH).
typedef VECTOR(Buffer) BufferList;

void buffer_list_destroy(BufferList *list);

void buffer_list_sort(BufferList *list);

Buffer buffer_list_longest_common_prefix(const BufferList *list);

BufferList buffer_split(const Buffer *self, const char *delim);

// convert list to a null (like the pointer) terminated array of c strings.
// this function consumes list. do not use it after calling this function
char **buffer_list_to_cstr_array(BufferList *list);

#endif
