#include "token.h"
#include "lib/error.h"

bool is_binary_op(TokenKind kind) {
  switch (kind) {
    case TK_SE_ADD:
    case TK_SE_SUB:
    case TK_SE_MUL:
    case TK_SE_POW:
    case TK_SE_DIV:
    case TK_SE_MOD:
    case TK_SE_EQ:
    case TK_SE_NEQ:
    case TK_SE_GT:
    case TK_SE_LT:
    case TK_SE_GTE:
    case TK_SE_LTE:
    case TK_SE_QUESTION:
      return true;
    default:
      return false;
    }

  unreachable();

  return false;
}
