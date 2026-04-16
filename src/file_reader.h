#ifndef FILE_READER_H
#define FILE_READER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lib/buffer.h"

typedef struct {
  FILE *file;
  Buffer line;
  bool eof;
} FileReader;

void file_reader_init(FileReader *file, FILE *fp);

const Buffer *file_reader_read(FileReader *file);

const Buffer *file_reader_read_void(void *file_ptr);

#endif
