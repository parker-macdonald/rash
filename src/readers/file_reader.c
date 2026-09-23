#include "file_reader.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#include "lib/buffer.h"
#include "readers/generic_reader.h"

typedef struct {
  FILE *file;
  Buffer line;
  bool eof;
} FileReader;

static const Buffer *file_reader_read(void *reader_ptr) {
  FileReader *self = reader_ptr;

  buffer_clear(&self->line);

  if (self->eof) {
    return NULL;
  }

  while(1) {
    int c = fgetc(self->file);

    if (c == '\n') {
      // don't spit out empty lines
      if (self->line.length == 0) {
        continue;
      }

      return &self->line;
    }

    if (c == EOF) {
      if (self->line.length == 0) {
        return NULL;
      }

      self->eof = true;
      return &self->line;
    }

    // don't capture control characters
    if (iscntrl(c)) {
      continue;
    }

    buffer_append(&self->line, (uint8_t)c);
  }
}

static void file_reader_destroy(void *reader_ptr) {
  FileReader *self = reader_ptr;

  if (fclose(self->file) == EOF) {
    perror("fclose");
  }

  buffer_destroy(&self->line);

  free(self);
}

int file_reader_create(GenericReader *out, const char *filepath) {
  FILE *fp = fopen(filepath, "r");

  if (fp == NULL) {
    return -1;
  }

  FileReader *reader = malloc(sizeof(FileReader));

  reader->file = fp;
  reader->eof = false;
  reader->line = buffer_create(0);

  out->reader_data = reader;
  out->read_line = file_reader_read;
  out->destroy = file_reader_destroy;

  return 0;
}
