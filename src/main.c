#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/find_builtin.h"
#include "file_reader.h"
#include "interactive.h"
#include "interpreter/evaluate.h"
#include "interpreter/lex.h"
#include "jobs.h"
#include "line_reader/line_reader.h"
#include "shell_vars.h"
#include "should_exit.h"

bool should_exit = false;
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
      return EXIT_FAILURE;
    }

    file_reader_init(file);
    reader = file_reader_read;
  } else {
    fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
    return EXIT_FAILURE;
  }

  int status = EXIT_SUCCESS;

  trie_init();
  sig_handler_init();

  set_var("PS1", "$ ");

  while (!should_exit) {
    const uint8_t *line = reader();
    clean_jobs();

    if (line == NULL) {
      status = EXIT_SUCCESS;
      break;
    }

    token_t *tokens = lex(line);

    if (tokens != NULL) {
      status = evaluate(tokens);
      free_tokens(&tokens);
    } else {
      status = EXIT_FAILURE;
    }

    if (should_exit) {
      break;
    }

    // 3 digit number (exit status is max of 255) + null terminator
    char status_str[3 + 1] = {0};

    snprintf(status_str, sizeof(status_str), "%d", status);
    set_var("?", status_str);
  }

  return status;
}
