#ifndef LEX_H
#define LEX_H

#include <stdint.h>

typedef enum {
  STRING,
  // '<' used to redirect stdin from a file
  STDIN_REDIR,
  // '<<<' used to redirect stdin from a string
  STDIN_REDIR_STRING,
  // '>' used to redirect stdout to a file, replace file contents
  STDOUT_REDIR,
  // '>>' used to redirect stdout to a file, append to file contents
  STDOUT_REDIR_APPEND,
  // '2>' used to redirect stderr to a file, replace file contents
  STDERR_REDIR,
  // '2>>' used to redirect stderr to a file, append to file contents
  STDERR_REDIR_APPEND,
  // '|' used to link one programs stdout to anothers stdin
  PIPE,
  // '*' matches zero or more characters while globbing
  GLOB_WILDCARD,
  // environment variable to be expanded
  ENV_EXPANSION,
  // subshell i.e. $(example string)
  SUBSHELL,
  // shell variable to be expanded
  VAR_EXPANSION,
  // '~' used for home folder expansion
  TILDE,
  // used to end the current argument
  END_ARG,
  // ';' used to run two commands sequentially
  SEMI,
  // '&&' used to run two commands sequencially, but only run the second if the
  // first is successful
  LOGICAL_AND,
  // '||' used to run two commands sequencially, but only run the second if the
  // first is unsuccessful
  LOGICAL_OR,
  // '&' run a program in the background
  AMP,
  // end a sequence of tokens
  END
} token_type_t;

#define IS_ARGUMENT_TOKENS(x)                                                  \
  ((x) == STRING || (x) == GLOB_WILDCARD || (x) == ENV_EXPANSION ||            \
   (x) == VAR_EXPANSION || (x) == TILDE || (x) == SUBSHELL)

typedef struct {
  token_type_t type;
  void *data;
} token_t;

token_t *lex(const uint8_t *source);

void free_tokens(token_t **tokens);

#endif
