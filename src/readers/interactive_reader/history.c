#include "history.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/vector.h"
#include "readers/interactive_reader/types.h"

void history_clear(InteractiveReader *reader) {
  VECTOR_DESTROY(reader->history);

  reader->history._capacity = 0;
  reader->history.length = 0;
  reader->history.data = NULL;

  reader->history_curr = 0;
}

void history_print(const InteractiveReader *reader, int count) {
  assert(count >= -1);

  if (count == 0) {
    return;
  }

  size_t i;

  if (count == -1) {
    i = 0;
  } else {
    i = (size_t)count;
  }

  for (; i < reader->history.length; i++) {
    printf(
        "%5zu  %.*s\n",
        i + 1,
        (int)reader->history.data[i].length,
        reader->history.data[i].char_ptr
    );
  }
}

void history_add(InteractiveReader *reader) {
  VECTOR_PUSH(reader->history, reader->buffer);
}

Buffer *history_curr(const InteractiveReader *reader) {
  assert(reader->history_curr < reader->history.length);

  return &reader->history.data[reader->history_curr];
}
