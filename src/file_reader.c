#include "file_reader.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#include "lib/vector.h"

void file_reader_init(FileReader *file, FILE *fp) {
  file->file = fp;
  file->eof = false;
  VECTOR_INIT(file->line);
}

const Buffer *file_reader_read(FileReader *file) {
  if (file->eof) {
    VECTOR_DESTROY(file->line);
    if (fclose(file->file) == EOF) {
      perror("fclose");
    }
    return NULL;
  }

  VECTOR_CLEAR(file->line);

  for (;;) {
    int c = fgetc(file->file);

    if (c == '\n') {
      if (file->line.length == 0) {
        continue;
      }

      return &file->line;
    }

    if (c == EOF) {
      if (file->line.length == 0) {
        VECTOR_DESTROY(file->line);
        if (fclose(file->file) == EOF) {
          perror("fclose");
        }
        return NULL;
      }

      file->eof = true;
      return &file->line;
    }

    // don't capture lexer reserved characters
    if (iscntrl(c)) {
      continue;
    }

    VECTOR_PUSH(file->line, (uint8_t)c);
  }
}

const Buffer *file_reader_read_void(void *file_ptr) {
  return file_reader_read((FileReader *)file_ptr);
}
