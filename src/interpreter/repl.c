#include "repl.h"

#include <stdint.h>
#include <stdlib.h>

#include "evaluate.h"
#include "jobs.h"
#include "lex.h"
#include "lib/buffer.h"
#include "rash.h"
#include "readers/generic_reader.h"

int repl(GenericReader *reader) {
  Rash *instance = rash_instance_get();

  while (1) {
    const Buffer *line = generic_reader_read(reader);

    jobs_clean(&instance->jobs);

    if (line == NULL) {
      break;
    }

    repl_once(line);
  }

  return 0;
}

int repl_once(const Buffer *line) {
  Rash *instance = rash_instance_get();

  int status = EXIT_SUCCESS;

  jobs_clean(&instance->jobs);

  // need to refactor lex to use a buffer instead of a null terminated string
  Buffer null_terminated_line = buffer_clone(line);
  buffer_append(&null_terminated_line, '\0');

  Token *tokens = lex(null_terminated_line.u8_ptr);

  buffer_destroy(&null_terminated_line);

  if (tokens != NULL) {
    status = evaluate(tokens);
    free_tokens(&tokens);
  } else {
    status = EXIT_FAILURE;
  }

  return status;
}
