#ifndef FILE_READER_H
#define FILE_READER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lib/vec_types.h"

struct file_reader {
  FILE *file;
  buf_t line;
  bool eof;
};

void file_reader_init(struct file_reader *file, FILE *fp);

const uint8_t *file_reader_read(void *file_ptr);

#endif
