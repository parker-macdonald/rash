#ifndef LEX_H
#define LEX_H

#include <stdint.h>

typedef enum {
  STRING = 0,
  STDIN_REDIR,
  STDIN_REDIR_STRING,
  STDOUT_REDIR,
  STDOUT_REDIR_APPEND,
  STDERR_REDIR,
  STDERR_REDIR_APPEND,
  PIPE,
  WILDCARD,
  QUESTION_MARK,
  BRACKET_OPEN,
  BRACKET_CLOSE,
  BRACE_OPEN,
  BRACE_CLOSE,
  DOUBLE_DOT,
  SEMI,
  LOGICAL_AND,
  LOGICAL_OR,
  END
} token_type_t;

extern const char *const TOKEN_NAMES[];

typedef struct {
  token_type_t type;
  void *data;
} token_t;

token_t *lex(const uint8_t *const source);

#endif
