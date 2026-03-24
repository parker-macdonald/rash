#include "line_reader.h"

#include <stdio.h>
#include <stdlib.h>

#include "lib/vector.h"
#include "line_reader_new/actions.h"
#include "line_reader_new/history.h"
#include "line_reader_struct.h"

line_reader *line_reader_create(void) {
  line_reader *reader = malloc(sizeof(line_reader));

  reader->buffer._capacity = 0;
  reader->buffer.length = 0;
  reader->buffer.data = 0;

  reader->buffer_offset = 0;

  reader->history_root = NULL;
  reader->history_end = NULL;
  reader->history_curr = NULL;

  reader->prompt = "$ ";
  reader->prompt_length = 2;

  reader->cursor_pos = reader->prompt_length;

  actions_default(&reader->acts);

  return reader;
}

void line_reader_destroy2(line_reader *reader) {
  VECTOR_DESTROY(reader->buffer);
  history_clear(reader);
}

const uint8_t *line_reader_read_void(void *reader) {
  return line_reader_read((line_reader *)reader);
}

const uint8_t *line_reader_read(line_reader *reader) {
  VECTOR_INIT(reader->buffer);

  printf("%s", reader->prompt);
  (void)fflush(stdout);

  while (1) {
    int status = preform_action(reader);

    if (status < 0) {
      return NULL;
    }

    if (status > 0) {
      break;
    }
  }

  return reader->buffer.data;
}
