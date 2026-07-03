#ifndef LEX_H
#define LEX_H

#include <stdint.h>

typedef enum {
  TK_STRING,
  // '<' used to redirect stdin from a file
  TK_STDIN_REDIR,
  // '<<<' used to redirect stdin from a string
  TK_STDIN_REDIR_STRING,
  // '>' used to redirect stdout to a file, replace file contents
  TK_STDOUT_REDIR,
  // '>>' used to redirect stdout to a file, append to file contents
  TK_STDOUT_REDIR_APPEND,
  // '2>' used to redirect stderr to a file, replace file contents
  TK_STDERR_REDIR,
  // '2>>' used to redirect stderr to a file, append to file contents
  TK_STDERR_REDIR_APPEND,
  // '|' used to link one programs stdout to anothers stdin
  TK_PIPE,
  // '*' matches zero or more characters while globbing
  TK_GLOB_WILDCARD,
  // environment variable to be expanded
  TK_ENV_EXPANSION,
  // subshell i.e. $(example string)
  TK_SUBSHELL,
  // shell variable to be expanded
  TK_VAR_EXPANSION,
  // '~' used for home folder expansion
  TK_TILDE,
  // used to end the current argument
  TK_END_ARG,
  // ';' used to run two commands sequentially
  TK_SEMI,
  // '&&' used to run two commands sequencially, but only run the second if the
  // first is successful
  TK_LOGICAL_AND,
  // '||' used to run two commands sequencially, but only run the second if the
  // first is unsuccessful
  TK_LOGICAL_OR,
  // '&' run a program in the background
  TK_AMP,
  // end a sequence of tokens
  TK_END
} TokenType;

#define IS_ARGUMENT_TOKENS(x)                                                  \
  ((x) == TK_STRING || (x) == TK_GLOB_WILDCARD || (x) == TK_ENV_EXPANSION ||   \
   (x) == TK_VAR_EXPANSION || (x) == TK_TILDE || (x) == TK_SUBSHELL)

typedef struct {
  TokenType type;
  void *data;
} Token;

Token *lex(const uint8_t *source);

void free_tokens(Token **tokens);

#endif
