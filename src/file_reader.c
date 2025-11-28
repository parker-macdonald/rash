#include "file_reader.h"

#include <stdbool.h>

#include "vector.h"

static FILE *file;

static VECTOR(uint8_t) line;

static bool eof;

void file_reader_init(FILE *fp) {
  file = fp;
  eof = false;
  VECTOR_INIT(line);
}

const uint8_t *file_reader_read(void) {
  if (eof) {
    VECTOR_DESTROY(line);
    if (fclose(file) == EOF) {
      perror("fclose");
    }
    return NULL;
  }

  for (;;) {
    int c = fgetc(file);

    if (c == '\n') {
      if (line.length == 0) {
        continue;
      }

      VECTOR_PUSH(line, '\0');
      VECTOR_CLEAR(line);
      return line.data;
    }

    if (c == EOF) {
      if (line.length == 0) {
        VECTOR_DESTROY(line);
        if (fclose(file) == EOF) {
          perror("fclose");
        }
        return NULL;
      }

      eof = true;
      VECTOR_PUSH(line, '\0');
      return line.data;
    }

    // don't capture lexer reserved characters
    if (c == '\0' || c == '\033') {
      continue;
    }

    VECTOR_PUSH(line, (uint8_t)c);
  }
}
