#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "builtins/find_builtin.h"
#include "interpreter/evaluate.h"
#include "interpreter/lex.h"
#include "jobs.h"
#include "line_reader/line_reader.h"
#include "shell_vars.h"
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

  set_var("PS1", "$ ");

  while (!should_exit) {
    const uint8_t *line = readline();
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

  fclose(file);

  return status;
}
