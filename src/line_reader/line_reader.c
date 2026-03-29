#include "line_reader.h"

#include <stdio.h>
#include <stdlib.h>

#include "lib/vector.h"
#include "line_reader/actions.h"
#include "line_reader/history.h"
#include "line_reader_struct.h"

static LineReader reader;

void line_reader_init(void) {
  reader.buffer._capacity = 0;
  reader.buffer.length = 0;
  reader.buffer.data = 0;

  reader.buffer_offset = 0;

  reader.active_buffer = &reader.buffer;

  reader.history_root = NULL;
  reader.history_end = NULL;
  reader.history_curr = NULL;

  reader.prompt = "$ ";
  reader.prompt_length = 2;

  reader.cursor_pos = reader.prompt_length;

  actions_default(&reader.acts);
}

void line_reader_destroy(void) {
  VECTOR_DESTROY(reader.buffer);
  history_clear(&reader);
}

const uint8_t *line_reader_read_void(void *_) {
  (void)_;

  return line_reader_read();
}

const uint8_t *line_reader_read(void) {
  VECTOR_INIT(reader.buffer);
  reader.active_buffer = &reader.buffer;
  reader.buffer_offset = 0;
  reader.cursor_pos = reader.prompt_length;

  printf("%s", reader.prompt);
  (void)fflush(stdout);

  while (1) {
    int status = preform_action(&reader);

    if (status < 0) {
      return NULL;
    }

    if (status > 0) {
      break;
    }
  }

  return reader.buffer.data;
}

void line_reader_hist_print(int count) {
  history_print(&reader, count);
}

void line_reader_hist_clear(void) {
  history_clear(&reader);
}
