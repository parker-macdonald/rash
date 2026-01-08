#include "file_reader.h"

#include <stdint.h>
#include <stdio.h>

#include "lib/vector.h"

void file_reader_init(struct file_reader *file, FILE *fp) {
  file->file = fp;
  file->eof = false;
  VECTOR_INIT(file->line);
}

const uint8_t *file_reader_read(void *file_ptr) {
  struct file_reader *file = file_ptr;
  if (file->eof) {
    VECTOR_DESTROY(file->line);
    if (fclose(file->file) == EOF) {
      perror("fclose");
    }
    return NULL;
  }

  for (;;) {
    int c = fgetc(file->file);

    if (c == '\n') {
      if (file->line.length == 0) {
        continue;
      }

      VECTOR_PUSH(file->line, '\0');
      VECTOR_CLEAR(file->line);
      return file->line.data;
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
      VECTOR_PUSH(file->line, '\0');
      return file->line.data;
    }

    // don't capture lexer reserved characters
    if (c == '\0' || c == '\033') {
      continue;
    }

    VECTOR_PUSH(file->line, (uint8_t)c);
  }
}
