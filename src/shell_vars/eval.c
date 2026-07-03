#include "eval.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "shell_vars.h"
#include "shell_vars/token.h"
#include <math.h>
#include <unistd.h>

typedef struct {
  const TokenList *tokens;
  size_t current;
} EvalState;

typedef enum {
  PREC_MIN = 0,
  PREC_EQ_NEQ,
  PREC_GT_LT_GTE_LTE,
  PREC_ADD_SUB,
  PREC_MUL_DIV,
  PREC_NOT,
  PREC_POW
} OpPrec;

const OpPrec OP_PREC_LOOKUP[] = {
  [TK_ADD] = PREC_ADD_SUB,
  [TK_SUB] = PREC_ADD_SUB,
  [TK_MUL] = PREC_MUL_DIV,
  [TK_DIV] = PREC_MUL_DIV,
  [TK_MOD] = PREC_MUL_DIV,
  [TK_POW] = PREC_POW,
  [TK_EQ]  = PREC_EQ_NEQ,
  [TK_NEQ] = PREC_EQ_NEQ,
  [TK_GT]  = PREC_GT_LT_GTE_LTE,
  [TK_LT]  = PREC_GT_LT_GTE_LTE,
  [TK_GTE] = PREC_GT_LT_GTE_LTE,
  [TK_LTE] = PREC_GT_LT_GTE_LTE
};

static bool is_at_end(EvalState *s) {
  return s->current >= s->tokens->length;
}

static Token advance(EvalState *s) {
  rash_panic(is_at_end(s));

  return s->tokens->data[s->current++];
}

static Token peek(EvalState *s) {
  rash_panic(is_at_end(s));

  return s->tokens->data[s->current];
}

static bool check(EvalState *s, TokenKind kind) {
  if (is_at_end(s)) {
    return false;
  }
  return peek(s).kind == kind;
}

static bool match(EvalState *s, TokenKind kind) {
  if (check(s, kind)) {
    advance(s);
    return true;
  }

  return false;
}

static ShellVar eval_term(EvalState *s);
static ShellVar eval_expr(EvalState *s, OpPrec prec);

static ShellVar eval_term(EvalState *s) {
  if (check(s, TK_NUMBER_LIT)) {
    Token tk = advance(s);
    return (ShellVar){.kind = SV_NUMBER, .number = tk.num_lit};
  }

  if (check(s, TK_STRING_LIT)) {
    Token tk = advance(s);
    return (ShellVar){.kind = SV_STRING, .string = buffer_clone(&tk.str_lit)};
  }

  if (check(s, TK_IDENTIFIER)) {
    Token tk = advance(s);
    ShellVar *var = var_get(buffer_cstr(&tk.identifier));

    if (var == NULL) {
      error("var is not set\n");
      _exit(420);
    }

    return *var;
  }

  if (match(s, TK_TRUE_LIT)) {
    return (ShellVar){.kind = SV_BOOLEAN, .boolean = true};
  }

  if (match(s, TK_FALSE_LIT)) {
    return (ShellVar){.kind = SV_BOOLEAN, .boolean = false};
  }

  if (match(s, TK_NULL_LIT)) {
    return (ShellVar){.kind = SV_NULL};
  }

  // unary plus
  if (match(s, TK_ADD)) {
    ShellVar var = eval_term(s);

    if (var.kind != SV_NUMBER) {
      // TODO: error handling
      error("unary plus can only be used on the number type\n");
      _exit(67);
    }

    return eval_term(s);
  }

  // unary minus
  if (match(s, TK_SUB)) {
    ShellVar var = eval_term(s);

    if (var.kind != SV_NUMBER) {
      // TODO: error handling
      error("unary minus can only be used on the number type\n");
      _exit(67);
    }

    var.number *= -1.0;

    return var;
  }

  if (match(s, TK_NOT)) {
    ShellVar var = eval_term(s);

    if (var.kind != SV_BOOLEAN) {
      // TODO: error handling
      error("not operator can only be used on the boolean type\n");
      _exit(67);
    }

    var.boolean = !var.boolean;

    return var;
  }

  if (match(s, TK_O_PAREN)) {
    ShellVar var = eval_expr(s, PREC_MIN);

    if (!match(s, TK_C_PAREN)) {
      error("no closing parem\n");
      _exit(2394);
    }

    return var;
  }

  rash_assert(1, "token error");
}

static ShellVar eval_expr(EvalState *s, OpPrec min_prec) {
  ShellVar left = eval_term(s);

  if (is_at_end(s)) {
    return left;
  }

  Token lookahead = peek(s);

  while (!is_at_end(s) && is_binary_op(lookahead.kind) && OP_PREC_LOOKUP[lookahead.kind] >= min_prec) {
    TokenKind op = lookahead.kind;
    advance(s);

    ShellVar right = eval_term(s);

    if (!is_at_end(s)) {
      lookahead = peek(s);
  
      while (is_binary_op(lookahead.kind) && OP_PREC_LOOKUP[lookahead.kind] > OP_PREC_LOOKUP[op]) {
        right = eval_expr(s, OP_PREC_LOOKUP[op] + 1);
        lookahead = peek(s);
      }
    }

    switch (op) {
      case TK_ADD:
        if (left.kind == SV_STRING) {
          Buffer right_str = var_to_string(&right);
          buffer_append_ptr(&left.string, right_str.void_ptr, right_str.length);
          buffer_destroy(&right_str);
          break;
        }
        if (right.kind == SV_STRING) {
          Buffer left_str = var_to_string(&left);
          buffer_append_ptr(&left_str, right.string.void_ptr, right.string.length);
          left.kind = SV_STRING;
          left.string = left_str;
          break;
        }

        if (left.kind == SV_NUMBER && right.kind == SV_NUMBER) {
          left.number += right.number;
          break;
        }

        rash_assert(1, "bad types");

      case TK_SUB:
        if (left.kind == SV_NUMBER && right.kind == SV_NUMBER) {
          left.number -= right.number;
          break;
        }

        rash_assert(1, "bad types");

      case TK_MUL:
        if (left.kind == SV_NUMBER && right.kind == SV_NUMBER) {
          left.number *= right.number;
          break;
        }

        rash_assert(1, "bad types");

      case TK_POW:
        if (left.kind == SV_NUMBER && right.kind == SV_NUMBER) {
          left.number = pow(left.number, right.number);
          break;
        }

        rash_assert(1, "bad types");

      case TK_DIV:
        if (left.kind == SV_NUMBER && right.kind == SV_NUMBER) {
          left.number /= right.number;
          break;
        }

        rash_assert(1, "bad types");

      case TK_MOD:
        if (left.kind == SV_NUMBER && right.kind == SV_NUMBER) {
          left.number = fmod(left.number, right.number);
          break;
        }

        rash_assert(1, "bad types");

      case TK_EQ:
        left.kind = SV_BOOLEAN;
        if (left.kind != right.kind) {
          left.boolean = false;
          break;
        }

        switch (left.kind) {
          case SV_NUMBER:
            left.boolean = left.number == right.number;
            break;
          case SV_STRING:
            left.boolean = buffer_compare(&left.string, &right.string) == 0;
            break;
          case SV_BOOLEAN:
            left.boolean = left.boolean == right.boolean;
          case SV_NULL:
            left.boolean = true;
            break;
        }
        break;
        
      case TK_NEQ:
        left.kind = SV_BOOLEAN;
        if (left.kind != right.kind) {
          left.boolean = true;
          break;
        }

        switch (left.kind) {
          case SV_NUMBER:
            left.boolean = left.number != right.number;
            break;
          case SV_STRING:
            left.boolean = buffer_compare(&left.string, &right.string) != 0;
            break;
          case SV_BOOLEAN:
            left.boolean = left.boolean != right.boolean;
          case SV_NULL:
            left.boolean = false;
            break;
        }
        break;

      case TK_GT:
        left.kind = SV_BOOLEAN;

        if (left.kind == SV_NUMBER && right.kind == SV_NUMBER) {
          left.boolean = left.number > right.number;
          break;
        }

        rash_assert(1, "bad types");

      case TK_LT:
        left.kind = SV_BOOLEAN;

        if (left.kind == SV_NUMBER && right.kind == SV_NUMBER) {
          left.boolean = left.number < right.number;
          break;
        }

        rash_assert(1, "bad types");

      case TK_GTE:
        left.kind = SV_BOOLEAN;

        if (left.kind == SV_NUMBER && right.kind == SV_NUMBER) {
          left.boolean = left.number >= right.number;
          break;
        }

        rash_assert(1, "bad types");

      case TK_LTE:
        left.kind = SV_BOOLEAN;

        if (left.kind == SV_NUMBER && right.kind == SV_NUMBER) {
          left.boolean = left.number <= right.number;
          break;
        }

        rash_assert(1, "bad types");
      default:
        rash_panic(1);
    }
  }

  return left;
}

ShellVar evaluate_tokens(const TokenList *tokens) {
  EvalState state = {.tokens = tokens, .current = 0};

  return eval_expr(&state, PREC_MIN);
}
