#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "builtins/find_builtin.h"
#include "file_reader.h"
#include "interactive.h"
#include "interpreter/repl.h"
#include "jobs.h"
#include "line_reader/line_reader.h"

volatile sig_atomic_t interactive = 0;

extern const char *const VERSION_STRING;
extern const char *const HELP_STRING;

int main(int argc, char **argv) {
  const uint8_t *(*reader)(void) = readline;

  if (argc == 1) {
    interactive = 1;
  } else if (argc == 2) {
    if (strcmp(argv[1], "--version") == 0) {
      puts(VERSION_STRING);
      return 0;
    }

    if (strcmp(argv[1], "--help") == 0) {
      puts(HELP_STRING);
      return 0;
    }

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
      fprintf(stderr, "rash: %s: %s\n", argv[1], strerror(errno));
      return 1;
    }

    file_reader_init(file);
    reader = file_reader_read;
  } else {
    fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
    return 1;
  }

  trie_init();
  sig_handler_init();

  if (interactive) {
    const char *home = getenv("HOME");

    if (home == NULL) {
      goto failure;
    }

    size_t len = strlen(home);

    char *rc_path = malloc(len + sizeof("/.rashrc"));

    if (rc_path == NULL) {
      goto failure;
    }

    memcpy(rc_path, home, len);
    strcpy(rc_path + len, "/.rashrc");

    FILE *rashrc = fopen(rc_path, "r");

    free(rc_path);

    if (rashrc == NULL) {
      if (errno != ENOENT) {
        perror("Failed to load .rash file");
      }

      goto failure;
    }

    file_reader_init(rashrc);

    repl(file_reader_read);
  }

failure:

  return repl(reader);
}
