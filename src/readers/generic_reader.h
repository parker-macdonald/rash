#ifndef READER_H
#define READER_H

#include "lib/buffer.h"
#include "readers/interactive_reader/types.h"

// sorta like a inheritance with a v-table kinda thing
typedef struct {
  void *reader_data;
  const Buffer *(*read_line)(void *reader_data);
  void (*destroy)(void *reader_data);
} GenericReader;

const Buffer *generic_reader_read(GenericReader *self);

void generic_reader_destroy(GenericReader *self);

void generic_reader_from_interactive(GenericReader *out, InteractiveReader *interactive);

#endif
