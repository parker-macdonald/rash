#ifndef FILE_READER_H
#define FILE_READER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lib/vec_types.h"

struct {
  FILE *file;
  Buffer line;
  bool eof;
} typedef FileReader;

void file_reader_init(FileReader *file, FILE *fp);

const uint8_t *file_reader_read(FileReader *file);

const uint8_t *file_reader_read_void(void *file_ptr);

#endif
