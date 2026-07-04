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

static ShellVar eval_term(EvalState *s);
static ShellVar eval_expr(EvalState *s, ShellVar lhs, OpPrec min_prec);
static ShellVar eval_part(ShellVar lhs, ShellVar rhs, TokenKind op);

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
    ShellVar var = eval_expr(s, eval_term(s), PREC_MIN);

    if (!match(s, TK_C_PAREN)) {
      error("no closing parem\n");
      _exit(2394);
    }

    return var;
  }

  rash_assert(1, "token error");
}

static ShellVar eval_part(ShellVar lhs, ShellVar rhs, TokenKind op) {
  switch (op) {
    case TK_ADD:
      if (lhs.kind == SV_STRING) {
        // promote rhs to a string
        Buffer right_str = var_to_string(&rhs);
        Buffer result = buffer_clone(&lhs.string);
        buffer_append_ptr(&result, right_str.void_ptr, right_str.length);
        
        return (ShellVar){.kind = SV_STRING, .string = result};
      }

      if (rhs.kind == SV_STRING) {
        Buffer result = var_to_string(&lhs);
        buffer_append_ptr(&result, rhs.string.void_ptr, rhs.string.length);
        
        return (ShellVar){.kind = SV_STRING, .string = result};
      }

      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return (ShellVar){
          .kind = SV_NUMBER,
          .number = lhs.number + rhs.number
        };
      }

      rash_assert(1, "bad types");

    case TK_SUB:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return (ShellVar){
          .kind = SV_NUMBER,
          .number = lhs.number - rhs.number
        };
      }

      rash_assert(1, "bad types");

    case TK_MUL:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return (ShellVar){
          .kind = SV_NUMBER,
          .number = lhs.number * rhs.number
        };
      }

      rash_assert(1, "bad types");

    case TK_POW:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return (ShellVar){
          .kind = SV_NUMBER,
          .number = pow(lhs.number, rhs.number)
        };
      }

      rash_assert(1, "bad types");

    case TK_DIV:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return (ShellVar){
          .kind = SV_NUMBER,
          .number = lhs.number / rhs.number
        };
      }

      rash_assert(1, "bad types");

    case TK_MOD:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return (ShellVar){
          .kind = SV_NUMBER,
          .number = fmod(lhs.number, rhs.number)
        };
      }

      rash_assert(1, "bad types");

    case TK_EQ:
      if (lhs.kind != rhs.kind) {
        return (ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = false
        };
      }

      switch (lhs.kind) {
        case SV_NUMBER:
          return (ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = lhs.number == rhs.number
          };
        case SV_STRING:
          return (ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = buffer_compare(&lhs.string, &rhs.string) == 0
          };
        case SV_BOOLEAN:
          return (ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = lhs.boolean == rhs.boolean
          };
        case SV_NULL:
          return (ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = true
          };
      }
      
    case TK_NEQ:
      if (lhs.kind != rhs.kind) {
        return (ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = true
        };
      }

      switch (lhs.kind) {
        case SV_NUMBER:
          return (ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = lhs.number != rhs.number
          };
        case SV_STRING:
          return (ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = buffer_compare(&lhs.string, &rhs.string) != 0
          };
        case SV_BOOLEAN:
          return (ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = lhs.boolean != rhs.boolean
          };
        case SV_NULL:
          return (ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = false
          };
      }

    case TK_GT:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return (ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = lhs.number > rhs.number
        };
      }

      rash_assert(1, "bad types");

    case TK_LT:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return (ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = lhs.number < rhs.number
        };
      }

      rash_assert(1, "bad types");

    case TK_GTE:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return (ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = lhs.number >= rhs.number
        };
      }

      rash_assert(1, "bad types");

    case TK_LTE:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return (ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = lhs.number <= rhs.number
        };
      }

      rash_assert(1, "bad types");
    default:
      rash_panic(1);
  }
}

static ShellVar eval_expr(EvalState *s, ShellVar lhs, OpPrec min_prec) {
  TokenKind lookahead = peek(s).kind;

  while (is_binary_op(lookahead) && OP_PREC_LOOKUP[lookahead] >= min_prec) {
    TokenKind op = lookahead;
    advance(s);

    ShellVar rhs = eval_term(s);

    lookahead = peek(s).kind;

    while (is_binary_op(lookahead) && OP_PREC_LOOKUP[lookahead] > OP_PREC_LOOKUP[op]) {
      rhs = eval_expr(s, rhs, OP_PREC_LOOKUP[op] + 1);

      lookahead = peek(s).kind;
    }

    lhs = eval_part(lhs, rhs, op);
  }

  return lhs;
}

ShellVar evaluate_tokens(const TokenList *tokens) {
  EvalState state = {.tokens = tokens, .current = 0};

  ShellVar lhs = eval_term(&state);
  return eval_expr(&state, lhs, PREC_MIN);
}
