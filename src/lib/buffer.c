#include "lib/buffer.h"

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "lib/error.h"
#include "lib/vector.h"
#include "lib/next_pow_2.h"

/*
 * Helper functions to create a new buffer
 */

// construct a buffer with length 0 and capacty of `next_pow_2(capacity)`
Buffer buffer_create(size_t capacity) {
  Buffer buffer;

  buffer.length = 0;

  if (capacity == 0) {
    buffer._capacity = 0;
    buffer.void_ptr = NULL;
    return buffer;
  }

  capacity = next_pow_2(capacity);
  buffer.void_ptr = malloc(capacity);
  buffer._capacity = capacity;

  rash_assert(buffer.void_ptr != NULL, "malloc failed");

  return buffer;
}

// constructs a buffer by copying the data at `data` of length `length` bytes.
Buffer buffer_from_ptr(const void *data, size_t length) {
  Buffer buffer;

  if (length == 0) {
    buffer._capacity = 0;
    buffer.length = 0;
    buffer.void_ptr = NULL;

    return buffer;
  }

  buffer._capacity = next_pow_2(length);
  buffer.length = length;
  buffer.void_ptr = malloc(buffer._capacity);

  memcpy(buffer.void_ptr, data, length);

  return buffer;
}

// constructs a buffer by copying the data at `cstr` of length `strlen(cstr)`.
Buffer buffer_from_cstr(const char *cstr) {
  size_t length = strlen(cstr);
  return buffer_from_ptr(cstr, length);
}

Buffer buffer_from_format(const char *format, ...) {
  va_list ap;
  va_start(ap, format);

  va_list ap2;
  va_copy(ap2, ap);
  int size = vsnprintf(NULL, 0, format, ap2);
  va_end(ap2);

  rash_assert(size >= 0, "vsnprintf failed");

  // must have space for a null terminator since vsnprintf will unconditionally write one
  Buffer buffer = buffer_create((size_t)size + 1);
  buffer.length = (size_t)size;

  int new_size = vsnprintf(buffer.char_ptr, (size_t)size + 1, format, ap);
  va_end(ap);

  rash_assert(size == new_size, "vsnprintf weirdness happened");

  return buffer;
}

// clone a buffer, creating a new one referencing a copy of the old ones data.
Buffer buffer_clone(const Buffer *other) {
  return buffer_from_ptr(other->void_ptr, other->length);
}

// destructor
void buffer_destroy(Buffer *self) {
  free(self->void_ptr);
  self->void_ptr = NULL;
  self->length = 0;
  self->_capacity = 0;
}

/*
 * Helper functions to append something to the end of an existing buffer
 */

void buffer_append_byte(Buffer *self, uint8_t byte) {
  buffer_append_ptr(self, &byte, 1);
}

void buffer_append_char(Buffer *self, char character) {
  buffer_append_ptr(self, &character, 1);
}

void buffer_append_ptr(Buffer *self, const void *data, size_t length) {
  buffer_grow_by(self, length);

  memcpy(self->u8_ptr + self->length, data, length);

  self->length += length;
}

void buffer_append_cstr(Buffer *self, const char *cstr) {
  size_t length = strlen(cstr);
  buffer_append_ptr(self, cstr, length);
}

void buffer_append_buffer(Buffer *self, const Buffer *other) {
  buffer_append_ptr(self, other->void_ptr, other->length);
}

/*
 * Helper functions to insert into an arbitrary place in an existing buffer
 */

void buffer_insert_ptr(Buffer *self, size_t at, const void *data,
                       size_t length) {
  rash_assert(at <= self->length, "at is greater than buffer length\n");

  buffer_grow_by(self, length);

  uint8_t *offset = self->u8_ptr + at;

  memmove(offset + length, offset, self->length - at);
  memcpy(offset, data, length);

  self->length += length;
}

void buffer_insert_byte(Buffer *self, size_t at, uint8_t byte) {
  buffer_insert_ptr(self, at, &byte, 1);
}

void buffer_insert_char(Buffer *self, size_t at, char character) {
  buffer_insert_ptr(self, at, &character, 1);
}

void buffer_insert_cstr(Buffer *self, size_t at, const char *cstr) {
  size_t length = strlen(cstr);
  buffer_insert_ptr(self, at, cstr, length);
}

void buffer_insert_buffer(Buffer *self, size_t at, const Buffer *other) {
  buffer_insert_ptr(self, at, other->void_ptr, other->length);
}

/*
 * Extra helper functions
 */

// remove n bytes from an arbitrary place in an existing buffer
void buffer_remove_n(Buffer *self, size_t at, size_t count) {
  rash_assert(self->length >= at + count, "index out of bounds on remove");

  size_t new_length = self->length - count;

  // removed from the end, no work needs to be done
  if (self->length == at) {
    return;
  }

  memmove(self->u8_ptr + at, self->u8_ptr + at + count,
          self->length - at - count);
  self->length = new_length;
}

// copy the contents of one buffer into another existing buffer
void buffer_copy(Buffer *self, const Buffer *other) {
  buffer_grow_to(self, other->length);

  memcpy(self->void_ptr, other->void_ptr, other->length);
  self->length = other->length;
}

// construct a buffer by "slicing" another from index `from` to index `to`.
// `from` is inclusive, `to` is exclusive. i.e. Buffer buffer =
// buffer_from_cstr("hi there"); Buffer slice = buffer_slice(3, buffer.length);
// slice is now "there"
Buffer buffer_slice(const Buffer *self, size_t from, size_t to) {
  return buffer_from_ptr(self->u8_ptr + from, to - from);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// similar to strcmp, but with buffers
int buffer_compare(const Buffer *self, const Buffer *other) {
  size_t min_length = MIN(self->length, other->length);

  for (size_t i = 0; i < min_length; i++) {
    int diff = self->u8_ptr[i] - other->u8_ptr[i];

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

int buffer_compare_cstr(const Buffer *self, const char *cstr) {
  // TODO: there's a way to do this without copying `cstr`, but i dont want to have to worry about it rn
  Buffer other = buffer_from_cstr(cstr);

  int saved = buffer_compare(self, &other);

  buffer_destroy(&other);

  return saved;
}

// resize the buffer to hold at least `grow_to` bytes. nothing is done if the
// buffer can already hold `grow_to` bytes
void buffer_grow_to(Buffer *self, size_t grow_to) {
  size_t new_capacity = next_pow_2(grow_to);

  if (new_capacity <= self->_capacity) {
    return;
  }

  self->_capacity = new_capacity;

  void *ptr = realloc(self->void_ptr, self->_capacity);

  rash_assert(ptr != NULL, "realloc failed\n");

  self->void_ptr = ptr;
}

// resize the buffer to hold at least `self->length + grow_by` bytes. nothing is
// done if the buffer can already hold `self->length + grow_by` bytes
void buffer_grow_by(Buffer *self, size_t grow_by) {
  buffer_grow_to(self, self->length + grow_by);
}

// modifies `self` by adding a null terminator but the length remains unchanged.
// this returns a pointer to the buffer's data casted to a `char *`
char *buffer_cstr(Buffer *self) {
  buffer_append_byte(self, '\0');

  self->length--;

  return self->char_ptr;
}

// set the length of a buffer to zero
void buffer_clear(Buffer *self) {
  self->length = 0;
}

size_t buffer_find_next(const Buffer *self, uint8_t search_for, size_t start_from) {
  assert(start_from < self->length);

  for (size_t i = start_from; i < self->length; i++) {
    if (self->u8_ptr[i] == search_for) {
      return i;
    }
  }

  return (size_t)-1;
}

size_t buffer_find_prev(const Buffer *self, uint8_t search_for, size_t start_from) {
  assert(start_from <= self->length);

  size_t i = start_from - 1;
  while (1) {
    if (self->u8_ptr[i] == search_for) {
      return i;
    }

    if (i == 0) {
      break;
    }

    i--;
  }

  return (size_t)-1;
}

bool buffer_contains_byte(const Buffer *self, uint8_t search_for) {
  for (size_t i = 0; i < self->length; i++) {
    if (self->u8_ptr[i] == search_for) {
      return true;
    }
  }

  return false;
}

bool buffer_starts_with_ptr(
  const Buffer *self,
  const void *starts_with,
  size_t starts_with_length
) {
  if (starts_with_length > self->length) {
    return false;
  }

  for (size_t i = 0; i < starts_with_length; i++) {
    if (self->u8_ptr[i] != ((uint8_t *)starts_with)[i]) {
      return false;
    }
  }

  return true;
}

bool buffer_starts_with_cstr(const Buffer *self, const char *starts_with) {
  return buffer_starts_with_ptr(self, starts_with, strlen(starts_with));
}

bool buffer_starts_with_buffer(const Buffer *self, const Buffer *starts_with) {
  return buffer_starts_with_ptr(self, starts_with->void_ptr, starts_with->length);
}

bool buffer_starts_with_char(const Buffer *self, char starts_with) {
  return buffer_starts_with_ptr(self, &starts_with, 1);
}

bool buffer_starts_with_byte(const Buffer *self, uint8_t starts_with) {
  return buffer_starts_with_ptr(self, &starts_with, 1);
}

// ---------- buffer list ------------

void buffer_list_destroy(BufferList *list) {
  for (size_t i = 0; i < list->length; i++) {
    buffer_destroy(list->data + i);
  }

  VECTOR_DESTROY(*list);
}

static int compare(const void *a, const void *b) {
  return buffer_compare((const Buffer *)a, (const Buffer *)b);
}

void buffer_list_sort(BufferList *list) {
  qsort(list->data, list->length, sizeof(Buffer), compare);
}

Buffer buffer_list_longest_common_prefix(const BufferList *list) {
  if (list->length == 0) {
    return buffer_create(0);
  }

  size_t min_length = list->data[0].length;

  for (size_t i = 1; i < list->length; i++) {
    if (list->data[i].length < min_length) {
      min_length = list->data[i].length;
    }
  }

  if (min_length == 0) {
    return buffer_create(0);
  }

  Buffer result = buffer_create(0);

  for (size_t i = 0; i < min_length; i++) {
    uint8_t byte = list->data[0].u8_ptr[i];

    for (size_t j = 1; j < list->length; j++) {
      if (byte != list->data[j].u8_ptr[i]) {
        return result;
      }
    }

    buffer_append_byte(&result, byte);
  }

  return result;
}

static bool char_is_one_of(char c, const char *accept) {
  size_t accept_len = strlen(accept);

  for (size_t i = 0; i < accept_len; i++) {
    if (c == accept[i]) {
      return true;
    }
  }

  return false;
}

// similar to strspn
static size_t buffer_prefix_length(const Buffer *self, size_t from, const char *accept) {
  if (from >= self->length) {
    return 0;
  }

  size_t i = from;

  while (1) {
    if (i == self->length) {
      break;
    }

    if (!char_is_one_of(self->char_ptr[i], accept)) {
      break;
    }

    i++;
  }

  return i;
}

BufferList buffer_split(const Buffer *self, const char *delim) {
  BufferList list = {0};

  size_t start = buffer_prefix_length(self, 0, delim);

  for (size_t i = start; i < self->length; i++) {
    if (char_is_one_of(self->char_ptr[i], delim)) {
      Buffer item = buffer_slice(self, start, i);
      VECTOR_PUSH(list, item);

      start = buffer_prefix_length(self, i, delim);
      i = start;
    }
  }

  return list;
}

char **buffer_list_to_cstr_array(BufferList *list) {
  // plus one for the null terminator
  char **array = malloc(sizeof(char *) * (list->length + 1));

  for (size_t i = 0; i < list->length; i++) {
    array[i] = buffer_cstr(list->data + i);
  }

  array[list->length] = NULL;

  VECTOR_DESTROY(*list);

  return array;
}
