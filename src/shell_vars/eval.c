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

static OptionShellVar eval_term(EvalState *s);
static OptionShellVar eval_expr(EvalState *s, ShellVar lhs, OpPrec min_prec);
static OptionShellVar eval_part(ShellVar lhs, ShellVar rhs, TokenKind op);

#define Some(x) ((OptionShellVar){.has_value = true, .value = (x)})
#define None ((OptionShellVar){.has_value = false})

static OptionShellVar eval_term(EvalState *s) { // NOLINT(misc-no-recursion)
  if (check(s, TK_NUMBER_LIT)) {
    Token tk = advance(s);
    return Some(((ShellVar){.kind = SV_NUMBER, .number = tk.num_lit}));
  }

  if (check(s, TK_STRING_LIT)) {
    Token tk = advance(s);
    return Some(((ShellVar){.kind = SV_STRING, .string = buffer_clone(&tk.str_lit)}));
  }

  if (check(s, TK_IDENTIFIER)) {
    Token tk = advance(s);
    ShellVar *var = var_get(buffer_cstr(&tk.identifier));

    if (var == NULL) {
      error_f("shell expression: var `%.*s` is not set.\n", (int)tk.identifier.length, tk.identifier.char_ptr);
      return None;
    }

    return Some(var_clone(var));
  }

  if (match(s, TK_TRUE_LIT)) {
    return Some(((ShellVar){.kind = SV_BOOLEAN, .boolean = true}));
  }

  if (match(s, TK_FALSE_LIT)) {
    return Some(((ShellVar){.kind = SV_BOOLEAN, .boolean = false}));
  }

  if (match(s, TK_NULL_LIT)) {
    return Some(((ShellVar){.kind = SV_NULL}));
  }

  // unary plus
  if (match(s, TK_ADD)) {
    OptionShellVar var = eval_term(s);

    if (!var.has_value) {
      error("shell expression: expected term following `+`.\n");
      return None;
    }

    if (var.value.kind != SV_NUMBER) {
      error("shell expression: unary plus can only be used on the number type.\n");
      var_destroy(&var.value);
      return None;
    }

    return eval_term(s);
  }

  // unary minus
  if (match(s, TK_SUB)) {
    OptionShellVar var = eval_term(s);

    if (!var.has_value) {
      error("shell expression: expected term following `-`.\n");
      return None;
    }

    if (var.value.kind != SV_NUMBER) {
      error("shell expression: unary minus can only be used on the number type.\n");
      var_destroy(&var.value);
      return None;
    }

    var.value.number *= -1.0;

    return var;
  }

  if (match(s, TK_NOT)) {
    OptionShellVar var = eval_term(s);

    if (!var.has_value) {
      error("shell expression: expected term following `!`.\n");
      return None;
    }

    if (var.value.kind != SV_BOOLEAN) {
      error("shell expression: not operator can only be used on the boolean type.\n");
      var_destroy(&var.value);
      return None;
    }

    var.value.boolean = !var.value.boolean;

    return var;
  }

  if (match(s, TK_O_PAREN)) {
    OptionShellVar lhs = eval_term(s);

    if (!lhs.has_value) {
      return None;
    }

    OptionShellVar var = eval_expr(s, lhs.value, PREC_MIN);
    var_destroy(&lhs.value);

    if (!var.has_value) {
      return None;
    }

    if (!match(s, TK_C_PAREN)) {
      error("shell expression: expected closing `)`.\n");
      var_destroy(&var.value);
      return None;
    }

    return var;
  }

  error_f("shell expression: expected term but found %s.\n", TOKEN_KIND_NAMES[peek(s).kind]);
  return None;
}

static OptionShellVar eval_part(ShellVar lhs, ShellVar rhs, TokenKind op) {
  switch (op) {
    case TK_ADD:
      if (lhs.kind == SV_STRING) {
        // promote rhs to a string
        Buffer right_str = var_to_string(&rhs);
        Buffer result = buffer_clone(&lhs.string);
        buffer_append_ptr(&result, right_str.void_ptr, right_str.length);
        
        return Some(((ShellVar){.kind = SV_STRING, .string = result}));
      }

      if (rhs.kind == SV_STRING) {
        Buffer result = var_to_string(&lhs);
        buffer_append_ptr(&result, rhs.string.void_ptr, rhs.string.length);
        
        return Some(((ShellVar){.kind = SV_STRING, .string = result}));
      }

      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return Some(((ShellVar){
          .kind = SV_NUMBER,
          .number = lhs.number + rhs.number
        }));
      }

      error_f("shell expression: cannot add types %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs.kind], SHELL_VAR_KIND_NAMES[rhs.kind]);
      return None;

    case TK_SUB:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return Some(((ShellVar){
          .kind = SV_NUMBER,
          .number = lhs.number - rhs.number
        }));
      }

      error_f("shell expression: can only subtract number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs.kind], SHELL_VAR_KIND_NAMES[rhs.kind]);
      return None;

    case TK_MUL:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return Some(((ShellVar){
          .kind = SV_NUMBER,
          .number = lhs.number * rhs.number
        }));
      }

      error_f("shell expression: can only multiply number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs.kind], SHELL_VAR_KIND_NAMES[rhs.kind]);
      return None;

    case TK_POW:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return Some(((ShellVar){
          .kind = SV_NUMBER,
          .number = pow(lhs.number, rhs.number)
        }));
      }

      error_f("shell expression: can only exponentiate number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs.kind], SHELL_VAR_KIND_NAMES[rhs.kind]);
      return None;

    case TK_DIV:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return Some(((ShellVar){
          .kind = SV_NUMBER,
          .number = lhs.number / rhs.number
        }));
      }

      error_f("shell expression: can only divide number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs.kind], SHELL_VAR_KIND_NAMES[rhs.kind]);
      return None;

    case TK_MOD:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return Some(((ShellVar){
          .kind = SV_NUMBER,
          .number = fmod(lhs.number, rhs.number)
        }));
      }

      error_f("shell expression: can only mod number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs.kind], SHELL_VAR_KIND_NAMES[rhs.kind]);
      return None;

    case TK_EQ:
      if (lhs.kind != rhs.kind) {
        return Some(((ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = false
        }));
      }

      switch (lhs.kind) {
        case SV_NUMBER:
          return Some(((ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = lhs.number == rhs.number
          }));
        case SV_STRING:
          return Some(((ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = buffer_compare(&lhs.string, &rhs.string) == 0
          }));
        case SV_BOOLEAN:
          return Some(((ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = lhs.boolean == rhs.boolean
          }));
        case SV_NULL:
          return Some(((ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = true
          }));
        default:
          unreachable();
      }
      
    case TK_NEQ:
      if (lhs.kind != rhs.kind) {
        return Some(((ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = true
        }));
      }

      switch (lhs.kind) {
        case SV_NUMBER:
          return Some(((ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = lhs.number != rhs.number
          }));
        case SV_STRING:
          return Some(((ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = buffer_compare(&lhs.string, &rhs.string) != 0
          }));
        case SV_BOOLEAN:
          return Some(((ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = lhs.boolean != rhs.boolean
          }));
        case SV_NULL:
          return Some(((ShellVar){
            .kind = SV_BOOLEAN,
            .boolean = false
          }));
        default:
          unreachable();
      }

    case TK_GT:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return Some(((ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = lhs.number > rhs.number
        }));
      }

      error_f("shell expression: can only compare number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs.kind], SHELL_VAR_KIND_NAMES[rhs.kind]);
      return None;

    case TK_LT:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return Some(((ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = lhs.number < rhs.number
        }));
      }

      error_f("shell expression: can only compare number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs.kind], SHELL_VAR_KIND_NAMES[rhs.kind]);
      return None;

    case TK_GTE:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return Some(((ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = lhs.number >= rhs.number
        }));
      }

      error_f("shell expression: can only compare number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs.kind], SHELL_VAR_KIND_NAMES[rhs.kind]);
      return None;

    case TK_LTE:
      if (lhs.kind == SV_NUMBER && rhs.kind == SV_NUMBER) {
        return Some(((ShellVar){
          .kind = SV_BOOLEAN,
          .boolean = lhs.number <= rhs.number
        }));
      }

      error_f("shell expression: can only compare number and number, not %s and %s.\n", SHELL_VAR_KIND_NAMES[lhs.kind], SHELL_VAR_KIND_NAMES[rhs.kind]);
      return None;
    default:
      unreachable();
  }
}

static OptionShellVar eval_expr(EvalState *s, ShellVar lhs, OpPrec min_prec) { // NOLINT(misc-no-recursion)
  TokenKind lookahead = peek(s).kind;
  OptionShellVar tmp;

  while (is_binary_op(lookahead) && OP_PREC_LOOKUP[lookahead] >= min_prec) {
    TokenKind op = lookahead;
    advance(s);

    tmp = eval_term(s);

    if (!tmp.has_value) {
      return None;
    }

    ShellVar rhs = tmp.value;

    lookahead = peek(s).kind;

    while (is_binary_op(lookahead) && OP_PREC_LOOKUP[lookahead] > OP_PREC_LOOKUP[op]) {
      tmp = eval_expr(s, rhs, OP_PREC_LOOKUP[op] + 1);
      var_destroy(&rhs);

      if (!tmp.has_value) {
        return None;
      }

      rhs = tmp.value;

      lookahead = peek(s).kind;
    }

    tmp = eval_part(lhs, rhs, op);
    var_destroy(&lhs);
    var_destroy(&rhs);

    if (!tmp.has_value) {
      return None;
    }

    lhs = tmp.value;
  }

  return Some(lhs);
}

OptionShellVar evaluate_tokens(const TokenList *tokens) {
  EvalState state = {.tokens = tokens, .current = 0};

  OptionShellVar lhs = eval_term(&state);

  if (!lhs.has_value) {
    return None;
  }

  OptionShellVar result = eval_expr(&state, lhs.value, PREC_MIN);
  var_destroy(&lhs.value);

  return result;
}
