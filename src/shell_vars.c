#include "shell_vars.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/hash_map.h"
#include "lib/parse.h"
#include "lib/slice.h"
#include "lib/vector.h"

static HashMap map;

static void var_destroy(void *ptr) {
  ShellVar *var = ptr;

  if (var->kind == SV_STRING) {
    buffer_destroy(&var->string);
  }
}

void var_init(void) {
  hash_map_init(&map, var_destroy);
}

void var_set(const char *key, const ShellVar *var) {
  ShellVar *var_clone = malloc(sizeof(ShellVar));
  memcpy(var_clone, var, sizeof(ShellVar));

  hash_map_set(&map, key, var_clone);
}

int var_unset(const char *const key) {
  hash_map_remove(&map, key);

  return 0;
}

static void print_callback(const char *key, void *ptr) {
  ShellVar *var = ptr;
  
  printf("{%s}:\t", key);

  switch (var->kind) {
  case SV_NUMBER:
    printf("%f (type: number)\n", var->number);
    break;
  case SV_STRING:
    printf("\"%.*s\" (type: string)\n", (int)var->string.length, var->string.char_ptr);
    break;
  case SV_BOOLEAN:
    printf("%s (type: boolean)\n", var->boolean ? "true" : "false");
    break;
  case SV_NULL:
    printf("null (type: null)\n");
    break;
  }
}

void var_print(void) {
  hash_map_iter(&map, print_callback);
}

ShellVar var_eval(const char *expr) {
  (void)expr;

  return (ShellVar){.kind = SV_NULL};
}

Buffer var_to_string(const ShellVar *var) {
  switch (var->kind) {
  case SV_NUMBER:
    return buffer_from_format("%g", var->number);
  case SV_STRING:
    return buffer_clone(&var->string);
  case SV_BOOLEAN:
    return buffer_from_format("%s", var->boolean ? "true" : "false");
  case SV_NULL:
    return buffer_from_cstr("null");
  }
}

char *var_eval_to_string(const char *expr) {
  ShellVar var = var_eval(expr);

  Buffer buffer = var_to_string(&var);

  var_destroy(&var);

  return buffer_cstr(&buffer);
}

typedef enum {
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

  TK_STRING_LIT,
  TK_NUMBER_LIT,
  TK_NULL_LIT,
  TK_TRUE_LIT,
  TK_FALSE_LIT,

  TK_IDENTIFIER
} TokenKind;

typedef struct {
  TokenKind kind;
  union {
    double num_lit;
    Buffer str_lit;
    Buffer identifier;
  };
} Token;

typedef VECTOR(Token) TokenList;

typedef struct {
  const Slice *source;
  TokenList tokens;
  size_t start;
  size_t current;
} LexState;

bool is_at_end(LexState *s) {
  return s->current >= s->source->length;
}

uint8_t advance(LexState *s) {
  s->current++;

  rash_panic(is_at_end(s));

  return s->source->u8_ptr[s->current];
}

uint8_t peek(LexState *s) {
  rash_panic(is_at_end(s));

  return s->source->u8_ptr[s->current];
}

uint8_t peek_next(LexState *s) {
  rash_panic(s->current + 1 >= s->source->length);
  
  return s->source->u8_ptr[s->current + 1];
}

bool match(LexState *s, uint8_t expected) {
  if (is_at_end(s)) {
    return false;
  }

  if (s->source->u8_ptr[s->current] != expected) {
    return false;
  }

  s->current++;
  return true;
}

void add_token(LexState *s, TokenKind kind) {
  VECTOR_PUSH(s->tokens, ((Token){.kind = kind}));
}

void add_string_literal(LexState *s, Buffer *value) {
  VECTOR_PUSH(
    s->tokens, 
    ((Token){
      .kind = TK_STRING_LIT,
      .str_lit = *value
    })
  );
}

void add_number_literal(LexState *s, double value) {
  VECTOR_PUSH(
    s->tokens, 
    ((Token){
      .kind = TK_NUMBER_LIT,
      .num_lit = value
    })
  );
}

void add_identifier(LexState *s, Buffer *identifier) {
  VECTOR_PUSH(
    s->tokens, 
    ((Token){
      .kind = TK_IDENTIFIER,
      .str_lit = *identifier
    })
  );
}

void string(LexState *s) {
  while (peek(s) != '"' && !is_at_end(s)) {
    advance(s);
  }

  if (is_at_end(s)) {
    error("shell expression: expected closing `\"` for string literal.\n");
    // TODO: error handling
    _exit(1);
    return;
  }

  // The closing ".
  advance(s);

  // Trim the surrounding quotes.
  Buffer value = buffer_from_ptr(
    s->source->u8_ptr + s->start + 1,
    s->current - s->start - 2
  );
  add_string_literal(s, &value);
}

void number(LexState *s) {
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
}

void identifier(LexState *s) {
  while (isalnum(peek(s))) {
    advance(s);
  }

  Buffer identifier = buffer_from_ptr(
    s->source->u8_ptr + s->start,
    s->current - s->start
  );

  if (buffer_compare_cstr(&identifier, "null") == 0) {
    add_token(s, TK_NULL_LIT);
    buffer_destroy(&identifier);
    return;
  }

  if (buffer_compare_cstr(&identifier, "true") == 0) {
    add_token(s, TK_TRUE_LIT);
    buffer_destroy(&identifier);
    return;
  }

  if (buffer_compare_cstr(&identifier, "false") == 0) {
    add_token(s, TK_FALSE_LIT);
    buffer_destroy(&identifier);
    return;
  }

  add_identifier(s, &identifier);
}

void scan_token(LexState *s) {
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
      add_token(s, TK_ADD);
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
      _exit(69);

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
        number(s);
        break;
      }
      if (isalpha((int)c)) {
        identifier(s);
        break;
      }

      // TODO: error handling
      error_f("unrecognized character `%c`.\n", c);
      _exit(1);
      break;
  }
}

TokenList lex(const Slice *source) {
  LexState state = {.source = source, .current = 0, .start = 0};
  VECTOR_INIT(state.tokens);

  while (!is_at_end(&state)) {
    // We are at the beginning of the next lexeme.
    state.start = state.current;
    scan_token(&state);
  }

  return state.tokens;
}
