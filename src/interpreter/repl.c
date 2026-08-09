#include "repl.h"

#include <stdint.h>
#include <stdlib.h>

#include "evaluate.h"
#include "interpreter/token.h"
#include "jobs.h"
#include "lex.h"
#include "lib/buffer.h"

int repl(const Buffer *(*reader)(void *), void *reader_data) {
  while (1) {
    const Buffer *line = reader(reader_data);

    if (line == NULL) {
      return 0;
    }

    repl_once(line);
  }

  return 0;
}

int repl_once(const Buffer *line) {
  int status = EXIT_SUCCESS;

  clean_jobs();

  TokenList tokens = lex(line);

  if (tokens.length != 0) {
    status = evaluate(&tokens);
    token_list_destroy(&tokens);
  } else {
    status = EXIT_FAILURE;
  }

  return status;
}
