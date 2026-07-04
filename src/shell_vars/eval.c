#include "eval.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "shell_vars.h"
#include "shell_vars/token.h"
#include <math.h>
#include <stdbool.h>
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
  if (is_at_end(s)) {
    return (Token){.kind = TK_NONE};
  }

  return s->tokens->data[s->current++];
}

static Token peek(EvalState *s) {
  if (is_at_end(s)) {
    return (Token){.kind = TK_NONE};
  }

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

static ShellVar *eval_term(EvalState *s);
static ShellVar *eval_expr(EvalState *s, ShellVar *lhs, OpPrec min_prec);
static ShellVar *eval_part(const ShellVar *lhs, const ShellVar *rhs, TokenKind op);

static ShellVar *eval_term(EvalState *s) { // NOLINT(misc-no-recursion)
  if (check(s, TK_NUMBER_LIT)) {
    Token tk = advance(s);
    return var_create_number(tk.num_lit);
  }

  if (check(s, TK_STRING_LIT)) {
    Token tk = advance(s);
    return var_create_string(buffer_clone(&tk.str_lit));
  }

  if (check(s, TK_IDENTIFIER)) {
    Token tk = advance(s);
    ShellVar *var = var_get(buffer_cstr(&tk.identifier));

    if (var == NULL) {
      error_f("shell expression: var `%.*s` is not set.\n", (int)tk.identifier.length, tk.identifier.char_ptr);
      return NULL;
    }

    return var;
  }

  if (match(s, TK_TRUE_LIT)) {
    return var_create_boolean(true);
  }

  if (match(s, TK_FALSE_LIT)) {
    return var_create_boolean(false);
  }

  if (match(s, TK_NULL_LIT)) {
    return var_create_null();
  }

  // unary plus
  if (match(s, TK_ADD)) {
    ShellVar *var = eval_term(s);

    if (var == NULL) {
      return NULL;
    }

    if (var->kind != SV_NUMBER) {
      error("shell expression: unary plus can only be used on the number type.\n");
      var_release(var);
      return NULL;
    }

    return eval_term(s);
  }

  // unary minus
  if (match(s, TK_SUB)) {
    ShellVar *var = eval_term(s);

    if (var == NULL) {
      return NULL;
    }

    if (var->kind != SV_NUMBER) {
      error("shell expression: unary minus can only be used on the number type.\n");
      var_release(var);
      return NULL;
    }

    var->number *= -1.0;

    return var;
  }

  if (match(s, TK_NOT)) {
    ShellVar *var = eval_term(s);

    if (var == NULL) {
      return NULL;
    }

    if (var->kind != SV_BOOLEAN) {
      error("shell expression: not operator can only be used on the boolean type.\n");
      var_release(var);
      return NULL;
    }

    var->boolean = !var->boolean;

    return var;
  }

  if (match(s, TK_O_PAREN)) {
    ShellVar *lhs = eval_term(s);

    if (lhs == NULL) {
      return NULL;
    }

    ShellVar *var = eval_expr(s, lhs, PREC_MIN);
    var_release(lhs);

    if (var == NULL) {
      return NULL;
    }

    if (!match(s, TK_C_PAREN)) {
      error("shell expression: expected closing `)`.\n");
      var_release(var);
      return NULL;
    }

    return var;
  }

  error_f("shell expression: expected term but found %s.\n", TOKEN_KIND_NAMES[peek(s).kind]);
  return NULL;
}

static ShellVar *eval_part(const ShellVar *lhs, const ShellVar *rhs, TokenKind op) {
  rash_assert(lhs == NULL || rhs == NULL, "bad arguments");

  switch (op) {
    case TK_ADD:
      if (lhs->kind == SV_STRING) {
        // promote rhs to a string
        Buffer right_str = var_to_string(rhs);
        Buffer result = buffer_clone(&lhs->string);
        buffer_append_buffer(&result, &right_str);
        
        return var_create_string(result);
      }

      if (rhs->kind == SV_STRING) {
        Buffer result = var_to_string(lhs);
        buffer_append_buffer(&result, &rhs->string);
        
        return var_create_string(result);;
      }

      if (lhs->kind == SV_NUMBER && rhs->kind == SV_NUMBER) {
        return var_create_number(lhs->number + rhs->number);
      }

      error_f("shell expression: cannot add types %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs->kind], SHELL_VAR_KIND_NAMES[rhs->kind]);
      return NULL;

    case TK_SUB:
      if (lhs->kind == SV_NUMBER && rhs->kind == SV_NUMBER) {
        return var_create_number(lhs->number - rhs->number);
      }

      error_f("shell expression: can only subtract number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs->kind], SHELL_VAR_KIND_NAMES[rhs->kind]);
      return NULL;

    case TK_MUL:
      if (lhs->kind == SV_NUMBER && rhs->kind == SV_NUMBER) {
        return var_create_number(lhs->number * rhs->number);
      }

      error_f("shell expression: can only multiply number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs->kind], SHELL_VAR_KIND_NAMES[rhs->kind]);
      return NULL;

    case TK_POW:
      if (lhs->kind == SV_NUMBER && rhs->kind == SV_NUMBER) {
        return var_create_number(pow(lhs->number, rhs->number));
      }

      error_f("shell expression: can only exponentiate number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs->kind], SHELL_VAR_KIND_NAMES[rhs->kind]);
      return NULL;

    case TK_DIV:
      if (lhs->kind == SV_NUMBER && rhs->kind == SV_NUMBER) {
        return var_create_number(lhs->number / rhs->number);
      }

      error_f("shell expression: can only divide number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs->kind], SHELL_VAR_KIND_NAMES[rhs->kind]);
      return NULL;

    case TK_MOD:
      if (lhs->kind == SV_NUMBER && rhs->kind == SV_NUMBER) {
        return var_create_number(fmod(lhs->number, rhs->number));
      }

      error_f("shell expression: can only mod number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs->kind], SHELL_VAR_KIND_NAMES[rhs->kind]);
      return NULL;

    case TK_EQ:
      if (lhs->kind != rhs->kind) {
        return var_create_boolean(false);
      }

      switch (lhs->kind) {
        case SV_NUMBER:
          return var_create_boolean(lhs->number == rhs->number);
        case SV_STRING:
          return var_create_boolean(buffer_compare(&lhs->string, &rhs->string) == 0);
        case SV_BOOLEAN:
          return var_create_boolean(lhs->boolean == rhs->boolean);
        case SV_NULL:
          return var_create_boolean(true);
        default:
          unreachable();
      }
      
    case TK_NEQ:
      if (lhs->kind != rhs->kind) {
        return var_create_boolean(true);
      }

      switch (lhs->kind) {
        case SV_NUMBER:
          return var_create_boolean(lhs->number != rhs->number);
        case SV_STRING:
          return var_create_boolean(buffer_compare(&lhs->string, &rhs->string) != 0);
        case SV_BOOLEAN:
          return var_create_boolean(lhs->boolean != rhs->boolean);
        case SV_NULL:
          return var_create_boolean(false);
        default:
          unreachable();
      }

    case TK_GT:
      if (lhs->kind == SV_NUMBER && rhs->kind == SV_NUMBER) {
        return var_create_boolean(lhs->number > rhs->number);
      }

      error_f("shell expression: can only compare number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs->kind], SHELL_VAR_KIND_NAMES[rhs->kind]);
      return NULL;

    case TK_LT:
      if (lhs->kind == SV_NUMBER && rhs->kind == SV_NUMBER) {
        return var_create_boolean(lhs->number < rhs->number);
      }

      error_f("shell expression: can only compare number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs->kind], SHELL_VAR_KIND_NAMES[rhs->kind]);
      return NULL;

    case TK_GTE:
      if (lhs->kind == SV_NUMBER && rhs->kind == SV_NUMBER) {
        return var_create_boolean(lhs->number >= rhs->number);
      }

      error_f("shell expression: can only compare number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs->kind], SHELL_VAR_KIND_NAMES[rhs->kind]);
      return NULL;

    case TK_LTE:
      if (lhs->kind == SV_NUMBER && rhs->kind == SV_NUMBER) {
        return var_create_boolean(lhs->number <= rhs->number);
      }

      error_f("shell expression: can only compare number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs->kind], SHELL_VAR_KIND_NAMES[rhs->kind]);
      return NULL;
    default:
      unreachable();
  }
}

static ShellVar *eval_expr(EvalState *s, ShellVar *lhs, OpPrec min_prec) { // NOLINT(misc-no-recursion)
  var_aquire(lhs);

  TokenKind lookahead = peek(s).kind;

  while (is_binary_op(lookahead) && OP_PREC_LOOKUP[lookahead] >= min_prec) {
    TokenKind op = lookahead;
    advance(s);

    ShellVar *rhs = eval_term(s);

    if (rhs == NULL) {
      var_release(lhs);
      return NULL;
    }

    lookahead = peek(s).kind;

    while (is_binary_op(lookahead) && OP_PREC_LOOKUP[lookahead] > OP_PREC_LOOKUP[op]) {
      ShellVar *tmp = eval_expr(s, rhs, OP_PREC_LOOKUP[op] + 1);
      var_release(rhs);

      if (tmp == NULL) {
        var_release(lhs);
        return NULL;
      }

      rhs = tmp;

      lookahead = peek(s).kind;
    }

    ShellVar *tmp = eval_part(lhs, rhs, op);
    var_release(lhs);
    var_release(rhs);

    if (tmp == NULL) {
      return NULL;
    }

    lhs = tmp;
  }

  return lhs;
}

ShellVar *evaluate_tokens(const TokenList *tokens) {
  EvalState state = {.tokens = tokens, .current = 0};

  ShellVar *lhs = eval_term(&state);

  if (lhs == NULL) {
    return NULL;
  }

  ShellVar *result = eval_expr(&state, lhs, PREC_MIN);
  var_release(lhs);

  return result;
}
