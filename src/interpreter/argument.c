#include "interpreter/argument.h"

#include "lib/buffer.h"
#include "lib/vector.h"

void argument_part_destroy(ArgumentPart *part) {
  if (
    part->kind == ARG_STRING ||
    part->kind == ARG_ENV ||
    part->kind == ARG_SHELL_EXPR ||
    part->kind == ARG_SUBSHELL ||
    part->kind == ARG_TILDE
  ) {
    buffer_destroy(&part->string);
  }
}

void argument_destroy(Argument *argument) {
  for (size_t i = 0; i < argument->length; i++) {
    ArgumentPart *part = argument->data + i;

    argument_part_destroy(part);
  }

  VECTOR_DESTROY(*argument);
}
