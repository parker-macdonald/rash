#ifndef INTERPRETER_ARGUMENT_H
#define INTERPRETER_ARGUMENT_H

#include "lib/buffer.h"
#include "lib/vector.h"
typedef enum {
  ARG_STRING,
  // '*' matches zero or more characters while globbing
  ARG_WILDCARD,
  // environment variable to be expanded
  ARG_ENV,
  // shell expression to be expanded i.e. {1 + 2}
  ARG_SHELL_EXPR,
  // subshell i.e. $(example string)
  ARG_SUBSHELL,
  // '~' used for home folder expansion
  ARG_TILDE
} ArgumentPartKind;

typedef struct {
  ArgumentPartKind kind;
  // defined for:
  // - ARG_STRING,
  // - ARG_ENV,
  // - ARG_SHELL_EXPR,
  // - ARG_SUBSHELL,
  // - ARG_TILDE
  Buffer string;
} ArgumentPart;

typedef VECTOR(ArgumentPart) Argument;

void argument_part_destroy(ArgumentPart *part);

void argument_destroy(Argument *argument);

#endif
