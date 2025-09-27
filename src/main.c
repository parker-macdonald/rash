#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/find_builtin.h"
#include "interpreter/lex.h"
#include "jobs.h"
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

    token_t *tokens = lex(line);

    for (size_t i = 0; tokens[i].type != END; i++) {
      printf("Token:\n  Type: %s\n", TOKEN_NAMES[tokens[i].type]);
      if (tokens[i].type == STRING) {
        printf("  Data: \"%s\"\n", (char *)tokens[i].data);
      }
    }
  }

  line_reader_destroy();

  fclose(file);

  trie_destroy();

  return status;
}
