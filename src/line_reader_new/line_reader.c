#include "line_reader.h"

#include <stdlib.h>

#include "lib/vector.h"
#include "line_reader_new/actions.h"
#include "line_reader_new/history.h"
#include "line_reader_struct.h"

line_reader *line_reader_create(void) {
  line_reader *reader = malloc(sizeof(line_reader));

  VECTOR_INIT(reader->buffer);

  reader->buffer_offset = 0;
  reader->cursor_pos = 0;

  reader->history_root = NULL;
  reader->history_end = NULL;
  reader->history_curr = NULL;

  actions_default(&reader->acts);

  return reader;
}

void line_reader_destroy2(line_reader *reader) {
  VECTOR_DESTROY(reader->buffer);
  // history_clear(reader);
}

void line_reader_print_hist(line_reader *reader) {
  (void)reader;
  // history_print(reader);
}

const uint8_t *line_reader_read_void(void *reader) {
  return line_reader_read((line_reader *)reader);
}

const uint8_t *line_reader_read(line_reader *reader) {
  while (preform_action(reader) == 0)
    ;

  return reader->buffer.data;
}
