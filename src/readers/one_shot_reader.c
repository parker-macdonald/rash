#include "one_shot_reader.h"
#include <stdlib.h>
#include "lib/buffer.h"
#include "readers/generic_reader.h"

typedef struct {
  Buffer line;
  bool has_read;
} OneShotReader;

static const Buffer *one_shot_reader_read(void *reader_ptr) {
  OneShotReader *self = reader_ptr;

  if (self->has_read) {
    return NULL;
  }

  self->has_read = true;
  return &self->line;
}

static void one_shot_reader_destroy(void *reader_ptr) {
  OneShotReader *self = reader_ptr;

  buffer_destroy(&self->line);
  free(self);
}

void one_shot_reader_create(GenericReader *out, Buffer line) {
  OneShotReader *reader = malloc(sizeof(OneShotReader));

  reader->line = line;
  reader->has_read = false;

  out->reader_data = reader;
  out->read_line = one_shot_reader_read;
  out->destroy = one_shot_reader_destroy;
}