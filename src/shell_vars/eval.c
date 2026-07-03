#include "eval.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "shell_vars.h"
#include "shell_vars/token.h"
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

// TODO: implement
static ShellVar eval_expr(EvalState *s, OpPrec prec) {
  (void)s;
  (void)prec;
  return (ShellVar){.kind = SV_NULL};
}

ShellVar evaluate_tokens(const TokenList *tokens) {
  EvalState state = {.tokens = tokens, .current = 0};

  return eval_term(&state);
}
