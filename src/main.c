#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/find_builtin.h"
#include "execute.h"
#include "jobs.h"
#include "lexer.h"
#include "line_reader.h"
#include "should_exit.h"

bool should_exit = false;

int main(int argc, char **argv) {
  FILE *file = NULL;

  if (argc == 2) {
    fprintf(stderr, "rash: Non-interactive mode is currently disabled\n");
    return EXIT_FAILURE;
  }
  if (argc == 1) {
    file = stdin;
  } else {
    fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
    return EXIT_FAILURE;
  }

  int status = EXIT_SUCCESS;

  trie_init();
  sig_handler_init();

  setenv("PS1", "$ ", 0);

  while (!should_exit) {
    const uint8_t *line = readline(getenv("PS1"));
    clean_jobs();

    if (line == NULL) {
      status = EXIT_SUCCESS;
      break;
    }

    char **tokens = get_tokens_from_line(line);

    if (tokens != NULL) {
      status = execute(tokens);

      free(tokens[0]);
      free(tokens);
    } else {
      status = EXIT_FAILURE;
    }

    if (should_exit) {
      break;
    }

    // 3 digit number (exit status is max of 255) + null terminator
    char status_env[3 + 1] = {0};

    sprintf(status_env, "%d", status);
    setenv("?", status_env, 1);
  }

  line_reader_destroy();

  fclose(file);

  trie_destroy();

  return status;
}
