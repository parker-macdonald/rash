#include "repl.h"

#include <stdint.h>
#include <stdlib.h>

#include "evaluate.h"
#include "jobs.h"
#include "lex.h"
#include "lib/buffer.h"

int repl(const Buffer *(*reader)(void *), void *reader_data) {
  while (1) {
    const Buffer *line = reader(reader_data);

    clean_jobs();

    if (line == NULL) {
      break;
    }

    // need to refactor lex to use a buffer instead of a null terminated string
    Buffer null_terminated_line = buffer_clone(line);
    buffer_append_byte(&null_terminated_line, '\0');

    Token *tokens = lex(null_terminated_line.data);

    buffer_destroy(&null_terminated_line);

    if (tokens != NULL) {
      evaluate(tokens);
      free_tokens(&tokens);
    }
  }

  return 0;
}

int repl_once(const Buffer *line) {
  int status = EXIT_SUCCESS;

  clean_jobs();

  // need to refactor lex to use a buffer instead of a null terminated string
  Buffer null_terminated_line = buffer_clone(line);
  buffer_append_byte(&null_terminated_line, '\0');

  Token *tokens = lex(null_terminated_line.data);

  buffer_destroy(&null_terminated_line);

  if (tokens != NULL) {
    status = evaluate(tokens);
    free_tokens(&tokens);
  } else {
    status = EXIT_FAILURE;
  }

  return status;
}
