#ifndef SHELL_VARS_TOKEN_H
#define SHELL_VARS_TOKEN_H

#include "lib/buffer.h"
#include "lib/vector.h"

#include <stdbool.h>

typedef enum {
  TK_NONE,
  
  TK_ADD, // +
  TK_SUB, // -
  TK_MUL, // *
  TK_POW, // **
  TK_DIV, // /
  TK_MOD, // %

  TK_EQ, // ==
  TK_NEQ, // !=
  TK_GT, // >
  TK_LT, // <
  TK_GTE, // >=
  TK_LTE, // <=
  TK_NOT, // !

  TK_O_PAREN, // (
  TK_C_PAREN, // )

  TK_STRING_TYPE,
  TK_BOOLEAN_TYPE,
  TK_NUMBER_TYPE,

  TK_STRING_LIT,
  TK_NUMBER_LIT,
  TK_NULL_LIT,
  TK_TRUE_LIT,
  TK_FALSE_LIT,

  TK_IDENTIFIER,
  TK_COUNT
} TokenKind;

extern const char *TOKEN_KIND_NAMES[TK_COUNT];

typedef struct {
  TokenKind kind;
  union {
    double num_lit;
    Buffer str_lit;
    Buffer identifier;
  };
} Token;

bool is_binary_op(TokenKind kind);

typedef VECTOR(Token) TokenList;

#endif
