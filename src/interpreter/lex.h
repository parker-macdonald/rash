#ifndef LEX_H
#define LEX_H

#include <stdint.h>

typedef enum {
  STRING = 1 << 0,
  // '<' used to redirect stdin from a file
  STDIN_REDIR = 1 << 1,
  // '<<<' used to redirect stdin from a string
  STDIN_REDIR_STRING= 1 << 2,
  // '>' used to redirect stdout to a file, replace file contents
  STDOUT_REDIR = 1 << 3,
  // '>>' used to redirect stdout to a file, append to file contents
  STDOUT_REDIR_APPEND = 1 << 4,
  // '2>' used to redirect stderr to a file, replace file contents
  STDERR_REDIR = 1 << 5,
  // '2>>' used to redirect stderr to a file, append to file contents
  STDERR_REDIR_APPEND = 1 << 6,
  // '|' used to link one programs stdout to anothers stdin
  PIPE = 1 << 7,
  // '*' matches zero or more characters while globbing
  GLOB_WILDCARD = 1 << 8,
  // environment variable to be expanded
  ENV_EXPANSION = 1 << 9,
  // shell variable to be expanded
  VAR_EXPANSION = 1 << 10,
  // used to end the current argument
  END_ARG = 1 << 11,
  // ';' used to run two commands sequentially
  SEMI = 1 << 12,
  // '&&' used to run two commands sequencially, but only run the second if the
  // first is successful
  LOGICAL_AND = 1 << 13,
  // '||' used to run two commands sequencially, but only run the second if the
  // first is unsuccessful
  LOGICAL_OR = 1 << 14,
  // '&' run a program in the background
  AMP = 1 << 15,
  // end a sequence of tokens
  END = 1 << 16,
} token_type_t;

#define ARGUMENT_TOKENS (STRING | GLOB_WILDCARD | ENV_EXPANSION | VAR_EXPANSION)

extern const char *const TOKEN_NAMES[];

typedef struct {
  token_type_t type;
  void *data;
} token_t;

token_t *lex(const uint8_t *source);

void free_tokens(token_t **tokens);

#endif
