#ifndef FILE_READER_H
#define FILE_READER_H

// this is pretty stupid, the source builtin and the main function each need
// their own file reader because, hypotheically, a shell script could call the
// source builtin, which tries to create a new file reader and tramples over the
// existing file reader in the main function (which crashes with a double free).
// if everything is declared in the header file, each translation unit that
// needs a file reader has one preallocated. i don't like it either, but we
// live.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "vector.h"

static struct {
  FILE *file;
  VECTOR(uint8_t) line;
  bool eof;
} DONT_USE;

static void file_reader_init(FILE *fp) {
  DONT_USE.file = fp;
  DONT_USE.eof = false;
  VECTOR_INIT(DONT_USE.line);
}

static const uint8_t *file_reader_read(void) {
  if (DONT_USE.eof) {
    VECTOR_DESTROY(DONT_USE.line);
    if (fclose(DONT_USE.file) == EOF) {
      perror("fclose");
    }
    return NULL;
  }

  for (;;) {
    int c = fgetc(DONT_USE.file);

    if (c == '\n') {
      if (DONT_USE.line.length == 0) {
        continue;
      }

      VECTOR_PUSH(DONT_USE.line, '\0');
      VECTOR_CLEAR(DONT_USE.line);
      return DONT_USE.line.data;
    }

    if (c == EOF) {
      if (DONT_USE.line.length == 0) {
        VECTOR_DESTROY(DONT_USE.line);
        if (fclose(DONT_USE.file) == EOF) {
          perror("fclose");
        }
        return NULL;
      }

      DONT_USE.eof = true;
      VECTOR_PUSH(DONT_USE.line, '\0');
      return DONT_USE.line.data;
    }

    // don't capture lexer reserved characters
    if (c == '\0' || c == '\033') {
      continue;
    }

    VECTOR_PUSH(DONT_USE.line, (uint8_t)c);
  }
}

#endif
