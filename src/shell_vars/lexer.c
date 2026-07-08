#include "lexer.h"
#include "lib/error.h"
#include "lib/parse.h"
#include "lib/vector.h"
#include "shell_vars/util.h"
#include "token.h"
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct {
  const Slice *source;
  TokenList tokens;
  size_t start;
  size_t current;
} LexState;

static bool is_at_end(LexState *s) {
  return s->current >= s->source->length;
}

static uint8_t advance(LexState *s) {
  rash_panic(is_at_end(s));
  
  return s->source->u8_ptr[s->current++];
}

static uint8_t peek(LexState *s) {
  if (is_at_end(s)) {
    return '\0';
  }

  return s->source->u8_ptr[s->current];
}

static uint8_t peek_next(LexState *s) {
  rash_panic(s->current + 1 >= s->source->length);
  
  return s->source->u8_ptr[s->current + 1];
}

static bool match(LexState *s, uint8_t expected) {
  if (is_at_end(s)) {
    return false;
  }

  if (s->source->u8_ptr[s->current] != expected) {
    return false;
  }

  s->current++;
  return true;
}

static void add_token(LexState *s, TokenKind kind) {
  VECTOR_PUSH(s->tokens, ((Token){.kind = kind}));
}

static void add_string_literal(LexState *s, Buffer *value) {
  VECTOR_PUSH(
    s->tokens, 
    ((Token){
      .kind = TK_STRING_LIT,
      .str_lit = *value
    })
  );
}

static void add_number_literal(LexState *s, double value) {
  VECTOR_PUSH(
    s->tokens, 
    ((Token){
      .kind = TK_NUMBER_LIT,
      .num_lit = value
    })
  );
}

static void add_identifier(LexState *s, Buffer *identifier) {
  VECTOR_PUSH(
    s->tokens, 
    ((Token){
      .kind = TK_IDENTIFIER,
      .str_lit = *identifier
    })
  );
}

static int string(LexState *s) {
  while (peek(s) != '"' && !is_at_end(s)) {
    advance(s);
  }

  if (is_at_end(s)) {
    error("shell expression: expected closing `\"` for string literal.\n");
    return -1;
  }

  // The closing ".
  advance(s);

  // Trim the surrounding quotes.
  Buffer value = buffer_from_ptr(
    s->source->u8_ptr + s->start + 1,
    s->current - s->start - 2
  );
  add_string_literal(s, &value);
  return 0;
}

static int number(LexState *s) {
  while (isdigit(peek(s))) advance(s);

  // Look for a fractional part.
  if (peek(s) == '.' && isdigit(peek_next(s))) {
    // Consume the "."
    advance(s);

    while (isdigit(peek(s))) advance(s);
  }

  Buffer number_str = buffer_from_ptr(
    s->source->u8_ptr + s->start,
    s->current - s->start
  );
  add_number_literal(s, parse_double(buffer_cstr(&number_str)).value);
  buffer_destroy(&number_str);

  return 0;
}

static int identifier(LexState *s) {
  while (is_ident(peek(s))) {
    advance(s);
  }

  Buffer identifier = buffer_from_ptr(
    s->source->u8_ptr + s->start,
    s->current - s->start
  );

  if (buffer_compare_cstr(&identifier, "null") == 0) {
    add_token(s, TK_NULL_LIT);
    buffer_destroy(&identifier);
    return 0;
  }

  if (buffer_compare_cstr(&identifier, "true") == 0) {
    add_token(s, TK_TRUE_LIT);
    buffer_destroy(&identifier);
    return 0;
  }

  if (buffer_compare_cstr(&identifier, "false") == 0) {
    add_token(s, TK_FALSE_LIT);
    buffer_destroy(&identifier);
    return 0;
  }

  add_identifier(s, &identifier);

  return 0;
}

static int scan_token(LexState *s) {
  uint8_t c = advance(s);

  switch (c) {
    case '(':
      add_token(s, TK_O_PAREN);
      break;

    case ')':
      add_token(s, TK_C_PAREN);
      break;

    case '+':
      add_token(s, TK_ADD);
      break;

    case '-':
      add_token(s, TK_SUB);
      break;

    case '*':
      if (match(s, '*')) {
        add_token(s, TK_POW);
        break;
      }
      add_token(s, TK_MUL);
      break;

    case '/':
      add_token(s, TK_DIV);
      break;

    case '%':
      add_token(s, TK_MOD);
      break;

    case '=':
      if (match(s, '=')) {
        add_token(s, TK_EQ);
        break;
      }

      // TODO: error handling
      error("shell expression: found stray `=`.\n");
      return -1;

    case '!':
      if (match(s, '=')) {
        add_token(s, TK_NEQ);
        break;
      }
      add_token(s, TK_NOT);
      break;

    case '>':
      if (match(s, '=')) {
        add_token(s, TK_GTE);
        break;
      }
      add_token(s, TK_GT);
      break;

    case '<':
      if (match(s, '=')) {
        add_token(s, TK_LTE);
        break;
      }
      add_token(s, TK_LT);
      break;

    case '"':
      string(s);
      break;

    case ' ':
    case '\r':
    case '\t':
    case '\n':
      // Ignore whitespace.
      break;

    default:
      if (isdigit((int)c)) {
        if (number(s)) {
          return -1;
        }
        break;
      }
      if (is_begin_ident(c)) {
        if (identifier(s)) {
          return -1;
        }
        break;
      }

      error_f("shell expression: unrecognized character `%c`.\n", c);
      return -1;
      break;
  }

  return 0;
}

TokenList lex_shell_expr(const Slice *source) {
  LexState state = {.source = source, .current = 0, .start = 0};
  VECTOR_INIT(state.tokens);

  while (!is_at_end(&state)) {
    // We are at the beginning of the next lexeme.
    state.start = state.current;
    if (scan_token(&state)) {
      VECTOR_DESTROY(state.tokens);
      return (TokenList){.length = 0, ._capacity = 0, .data = NULL};
    }
  }

  return state.tokens;
}
