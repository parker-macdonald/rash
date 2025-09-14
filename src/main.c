#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/find_builtin.h"
#include "execute.h"
#include "jobs.h"
#include "lexer.h"
#include "line_reader/line_reader.h"
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
    const uint8_t *line = readline();
    clean_jobs();

    if (line == NULL) {
      status = EXIT_SUCCESS;
      break;
    }

    optional_exec_context context = get_tokens_from_line(line);

    if (context.has_value) {
      status = execute(context.value);

      free(context.value.argv[0]);
      free(context.value.argv);
    } else {
      status = EXIT_FAILURE;
    }

    if (should_exit) {
      break;
    }

    // 3 digit number (exit status is max of 255) + null terminator
    char status_env[3 + 1] = {0};

    snprintf(status_env, sizeof(status_env), "%d", status);
    setenv("?", status_env, 1);
  }

  line_reader_destroy();

  fclose(file);

  trie_destroy();

  return status;
}
