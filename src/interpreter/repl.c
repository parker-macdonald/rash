#include "repl.h"

#include <stdint.h>
#include <stdlib.h>

#include "evaluate.h"
#include "interpreter/token.h"
#include "jobs.h"
#include "lex.h"
#include "lib/buffer.h"
#include "global.h"
#include "readers/generic_reader.h"

int repl(GenericReader *reader) {
  while (1) {
    const Buffer *line = generic_reader_read(reader);

    jobs_clean(&instance.jobs);

    if (line == NULL) {
      break;
    }

    repl_once(line);
  }

  return 0;
}

int repl_once(const Buffer *line) {
  int status = EXIT_SUCCESS;

  jobs_clean(&instance.jobs);

  TokenList tokens = lex(line);

  if (tokens.length != 0) {
    status = evaluate(&tokens);
  } else {
    status = EXIT_FAILURE;
  }
  
  token_list_destroy(&tokens);

  return status;
}
