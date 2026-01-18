#include "repl.h"

#include <stdint.h>
#include <stdlib.h>

#include "evaluate.h"
#include "jobs.h"
#include "lex.h"

int repl(const uint8_t *(*reader)(void *), void *reader_data) {
  while (1) {
    const uint8_t *line = reader(reader_data);
    
    clean_jobs();

    if (line == NULL) {
      break;
    }

    token_t *tokens = lex(line);

    if (tokens != NULL) {
      evaluate(tokens);
      free_tokens(&tokens);
    }
  }

  return 0;
}

int repl_once(const uint8_t *line) {
  int status = EXIT_SUCCESS;

  clean_jobs();

  token_t *tokens = lex(line);

  if (tokens != NULL) {
    status = evaluate(tokens);
    free_tokens(&tokens);
  } else {
    status = EXIT_FAILURE;
  }

  return status;
}
