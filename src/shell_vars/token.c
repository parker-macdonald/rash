#include "token.h"

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
    case TK_NOT:
    case TK_O_PAREN:
    case TK_C_PAREN:
    case TK_STRING_LIT:
    case TK_NUMBER_LIT:
    case TK_NULL_LIT:
    case TK_TRUE_LIT:
    case TK_FALSE_LIT:
    case TK_IDENTIFIER:
      return false;
  }
}
