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
#include "one_shot.h"
#include "strings/strings.h"

volatile sig_atomic_t interactive = 0;

int main(int argc, char **argv) {
  const uint8_t *(*reader)(void) = readline;

  // no arguments means interactive mode
  if (argc == 1) {
    interactive = 1;
  } else if (argc == 2) {
    if (strcmp(argv[1], "--version") == 0) {
      puts(VERSION_STRING);
      return 0;
    }

    if (strcmp(argv[1], "--help") == 0) {
      printf(
          "Usage: %s [-c] [FILENAME]\n"
          "If a filename is specified, rash will run the file as a script.\n"
          "If no filename is specified rash will run in interactive mode.\n"
          "The -c option specifies one-shot mode, rash will run one command \n"
          "specified in the next argument, then exit.\n"
          "For example: \n"
          "  rash -c 'echo hello'\n"
          "rash will run 'echo hello', then exit.\n",
          argv[0]
      );
      return 0;
    }

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
      fprintf(stderr, "rash: %s: %s\n", argv[1], strerror(errno));
      return 1;
    }

    file_reader_init(file);
    reader = file_reader_read;
  } else if (argc == 3) {
    // one-shot mode
    if (strcmp(argv[1], "-c") != 0) {
      fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
      return 1;
    }

    one_shot_init((uint8_t*)argv[2]);
    reader = one_shot_reader;
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
