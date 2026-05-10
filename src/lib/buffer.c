#include "lib/buffer.h"
#include "lib/next_pow_2.h"

#include <stdlib.h>
#include <string.h>

/*
 * Helper functions to create a new buffer
 */

Buffer buffer_from_ptr(const void *data, size_t length) {
  Buffer buffer;

  buffer._capacity = next_pow_2(length);
  buffer.data = malloc(buffer._capacity);
  buffer.length = length;

  memcpy(buffer.data, data, length);

  return buffer;
}

Buffer buffer_from_cstr(const char *cstr) {
  size_t length = strlen(cstr);

  return buffer_from_ptr(cstr, length);
}

Buffer buffer_from_span(const Span *span) {
  return buffer_from_ptr(span->data, span->length);
}

Span buffer_slice(const Buffer *self, size_t from, size_t to) {
  return span_from_ptr(self->data + from, to - from);
}

Buffer buffer_clone(const Buffer *other) {
  return buffer_from_ptr(other->data, other->length);
}

void buffer_destroy(Buffer *self) {
  free(self->data);

  self->data = NULL;
  self->length = 0;
  self->_capacity = 0;
}

/*
 * Helper functions to append something to the end of an existing buffer
 */

void buffer_append_byte(Buffer *self, uint8_t byte) {
  if (self->_capacity <= self->length) {
    if (self->_capacity == 0) {
      self->_capacity = self->length + 1;
    } else {
      self->_capacity *= 2;
    }

    void *data = realloc(self->data, sizeof(*self->data) * self->_capacity);
    if (data == NULL) {
      abort();
    }
    self->data = data;
  }

  self->data[self->length] = byte;
  self->length++;
}

void buffer_append_ptr(Buffer *self, const void *data, size_t length) {
  size_t new_length = length + self->length;

  if (self->_capacity < new_length) {
    self->_capacity = next_pow_2(new_length);
    void *new_data = realloc(self->data, self->_capacity);
    if (new_data == NULL) {
      abort();
    }
    self->data = new_data;
  }

  uint8_t *line_end = self->data + self->length;

  memcpy(line_end, data, length);

  self->length = new_length;
}

void buffer_append_cstr(Buffer *self, const char *cstr) {
  size_t length = strlen(cstr);

  buffer_append_ptr(self, cstr, length);
}

void buffer_append_span(Buffer *self, const Span *span) {
  buffer_append_ptr(self, span->data, span->length);
}

/*
 * Helper functions to insert into an arbitrary place in an existing buffer
 */

void buffer_insert_byte(Buffer *self, size_t at, uint8_t byte) {
  if (at >= self->length) {
    buffer_append_byte(self, byte);
    return;
  }

  uint8_t old_value = self->data[at];
  self->data[at] = byte;

  for (size_t i = at + 1; i < self->length; i++) {
    const uint8_t saved = self->data[i];

    self->data[i] = old_value;

    old_value = saved;
  }

  buffer_append_byte(self, old_value);
}

void buffer_insert_cstr(Buffer *self, size_t at, const char *cstr) {
  size_t length = strlen(cstr);

  buffer_insert_ptr(self, at, cstr, length);
}

void buffer_insert_span(Buffer *self, size_t at, const Span *span) {
  buffer_insert_ptr(self, at, span->data, span->length);
}

void buffer_insert_ptr(Buffer *self, size_t at, const void *data,
                       size_t length) {
  size_t new_length = length + self->length;

  if (self->_capacity < new_length) {
    self->_capacity = next_pow_2(new_length);
    void *new_data = realloc(self->data, self->_capacity);
    if (new_data == NULL) {
      abort();
    }
    self->data = new_data;
  }

  if (at == self->length) {
    uint8_t *line_end = self->data + self->length;

    memcpy(line_end, data, length);

    self->length = new_length;
    return;
  }

  uint8_t *offset = self->data + at;

  memmove(offset + length, offset, self->length - at);

  // copy new data in
  memcpy(offset, data, length);

  self->length = new_length;
}

/*
 * Other useful functions for dealing with buffers
 */

void buffer_remove_n(Buffer *buffer, size_t at, size_t count) {
  buffer->length -= count;

  if (buffer->length == at + count) {
    return;
  }

  memmove(
    buffer->data, 
    buffer->data + count, 
    buffer->length - at
  );
}

void buffer_copy(Buffer *dest, const Buffer *src) {
  if (dest->_capacity < src->length) {
    free(dest->data);
    dest->_capacity = next_pow_2(src->length);
    dest->data = malloc(dest->_capacity);
  }

  dest->length = src->length;
  memcpy(dest->data, src->data, src->length);
}

void buffer_replace(Buffer *self, uint8_t search_for, uint8_t replace_with) {
  for (size_t i = 0; i < self->length; i++) {
    if (self->data[i] == search_for) {
      self->data[i] = replace_with;
    }
  }
}

/*
 * Helper functions to create a new slice
 */

Span span_from_ptr(const void *data, size_t length) {
  return (Span){.data = data, .length = length};
}

Span span_from_cstr(const char *cstr) {
  size_t length = strlen(cstr);

  return span_from_ptr(cstr, length);
}

Span span_from_buffer(const Buffer *buffer) {
  return (Span){.data = buffer->data, .length = buffer->length};
}

bool span_contains(const Span *self, uint8_t search_for) {
  for (size_t i = 0; i < self->length; i++) {
    if (self->data[i] == search_for) {
      return true;
    }
  }

  return false;
}

size_t span_index_of(const Span *self, uint8_t search_for) {
  for (size_t i = 0; i < self->length; i++) {
    if (self->data[i] == search_for) {
      return i;
    }
  }

  return (size_t)-1;
}

size_t span_last_index_of(const Span *self, uint8_t search_for) {
  size_t location = (size_t)-1;

  for (size_t i = 0; i < self->length; i++) {
    if (self->data[i] == search_for) {
      location = i;
    }
  }

  return location;
}

// similar to strcmp, but with spans
int span_compare(const Span *s1, const Span *s2) {
  size_t min_length = s1->length > s2->length ? s2->length : s1->length;

  for (size_t i = 0; i < min_length; i++) {
    int diff = s1->data[i] - s2->data[i];

    if (diff != 0) {
      return diff;
    }
  }

  if (s1->length < s2->length) {
    return -1;
  }
  if (s1->length > s2->length) {
    return 1;
  }

  return 0;
}
