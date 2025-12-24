#include "repl.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../jobs.h"
#include "../shell_vars.h"
#include "evaluate.h"
#include "lex.h"
#include "preprocess.h"

int repl(const uint8_t *(*reader)(void *), void *reader_data) {
  int status = EXIT_SUCCESS;

  while (1) {
    const uint8_t *line = reader(reader_data);
    clean_jobs();

    if (line == NULL) {
      status = EXIT_SUCCESS;
      break;
    }

    uint8_t *processed_line = preprocess(line);
    if (processed_line == NULL) {
      status = EXIT_FAILURE;
      goto stop_evaluating;
    }

    token_t *tokens = lex(processed_line);
    free(processed_line);

    if (tokens == NULL) {
      status = EXIT_FAILURE;
      goto stop_evaluating;
    }

    status = evaluate(tokens);
    free_tokens(&tokens);

  stop_evaluating: {
    // 3 digit number (exit status is max of 255) + null terminator
    char status_str[3 + 1] = {0};

    snprintf(status_str, sizeof(status_str), "%d", status);
    var_set("?", status_str);
  }
  }

  return status;
}
