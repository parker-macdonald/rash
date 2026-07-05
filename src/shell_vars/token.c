#include "token.h"

const char *TOKEN_KIND_NAMES[TK_COUNT] = {
  [TK_NONE] = "None",
  [TK_ADD] = "`+`",
  [TK_SUB] = "`-`",
  [TK_MUL] = "`*`",
  [TK_POW] = "`**`",
  [TK_DIV] = "`/`",
  [TK_MOD] = "`%`",
  [TK_EQ] = "`==`",
  [TK_NEQ] = "`!=`",
  [TK_GT] = "`>`",
  [TK_LT] = "`<`",
  [TK_GTE] = "`>=`",
  [TK_LTE] = "`<=`",
  [TK_NOT] = "`!`",
  [TK_O_PAREN] = "`(`",
  [TK_C_PAREN] = "`)`",
  [TK_STRING_LIT] = "string literal",
  [TK_NUMBER_LIT] = "number literal",
  [TK_NULL_LIT] = "`null`",
  [TK_TRUE_LIT] = "`true`",
  [TK_FALSE_LIT] = "`false`",
  [TK_IDENTIFIER] = "identifier"
};

bool is_binary_op(TokenKind kind) {
  switch (kind) {
    case TK_ADD:
    case TK_SUB:
    case TK_MUL:
    case TK_POW:
    case TK_DIV:
    case TK_MOD:
    case TK_EQ:
    case TK_NEQ:
    case TK_GT:
    case TK_LT:
    case TK_GTE:
    case TK_LTE:
      return true;
    case TK_NONE:
    case TK_NOT:
    case TK_O_PAREN:
    case TK_C_PAREN:
    case TK_STRING_LIT:
    case TK_NUMBER_LIT:
    case TK_NULL_LIT:
    case TK_TRUE_LIT:
    case TK_FALSE_LIT:
    case TK_IDENTIFIER:
    case TK_COUNT:
      return false;
  }
}
