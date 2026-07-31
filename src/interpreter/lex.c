#include "interpreter/lex.h"

#include "lib/attrib.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/vector.h"

#include "token.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

typedef struct {
  const Buffer *source;
  TokenList tokens;
  size_t start;
  size_t current;
} LexState;

static bool is_at_end(LexState *s) {
  return s->current >= s->source->length;
}

static uint8_t advance(LexState *s) {
  if (is_at_end(s)) {
    rash_panic();
  }

  return s->source->u8_ptr[s->current++];
}

static uint8_t peek(LexState *s) {
  if (is_at_end(s)) {
    return '\0';
  }

  return s->source->u8_ptr[s->current];
}

ATTRIB_UNUSED
static uint8_t peek_next(LexState *s) {
  if (s->current + 1 >= s->source->length) {
    rash_panic();
  }

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

static bool match_many(LexState *s, const char *expected) {
  size_t i;
  for (i = 0; expected[i] != '\0'; i++) {
    if (
      is_at_end(s) ||
      s->source->char_ptr[s->current + i] != expected[i]
    ) {
      return false;
    }
  }

  s->current += i;
  return true;
}

static bool is_last_token_argument(LexState *s) {
  if (
    s->tokens.length != 0 &&
    IS_ARGUMENT_TOKEN(s->tokens.data[s->tokens.length - 1].kind)
  ) {
    return true;
  }

  return false;
}

static void add_token(LexState *s, TokenKind kind) {
  // if we insert a non-argument token following an argument token, we need an ARG_END_TOKEN
  if (
    !IS_ARGUMENT_TOKEN(kind) &&
    is_last_token_argument(s)
  ) {
    VECTOR_PUSH(s->tokens, ((Token){.kind = TK_ARG_END}));
  }

  VECTOR_PUSH(s->tokens, ((Token){.kind = kind}));
}

static void add_buffer_token(LexState *s, TokenKind kind, Buffer buffer) {
  assert(IS_BUFFER_TOKEN(kind));

  if (
    !IS_ARGUMENT_TOKEN(kind) &&
    is_last_token_argument(s)
  ) {
    VECTOR_PUSH(s->tokens, ((Token){.kind = TK_ARG_END}));
  }

  VECTOR_PUSH(s->tokens, ((Token){
    .kind = kind,
    .buffer = buffer
  }));
}

// what is and isn't allowed in a username isn't set in stone. i'm limiting it
// to lower and upper case ASCII letters, digits, period, underscore, and hyphen
// see here: https://systemd.io/USER_NAMES/
static bool is_username_allowed(uint8_t c) {
  return (
    (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') ||
    (c == '.') ||
    (c == '_') ||
    (c == '-')
  );
}

static int tilde(LexState *s) {
  while (is_username_allowed(peek(s))) {
    advance(s);
  }

  Buffer username = buffer_slice(
    s->source,
    s->start + 1,
    s->current
  );

  add_buffer_token(s, TK_ARG_TILDE, username);
  return 0;
}

static int scan_token(LexState *s) {
  // stdin redirects
  if (match_many(s, "<<<")) {
    add_token(s, TK_STDIN_REDIR_STRING);
    return 0;
  }

  if (match(s, '<')) {
    add_token(s, TK_STDIN_REDIR);
    return 0;
  }

  // stdout redirects
  if (match_many(s, ">>")) {
    add_token(s, TK_STDOUT_REDIR_APPEND);
    return 0;
  }

  if (match(s, '>')) {
    add_token(s, TK_STDOUT_REDIR);
    return 0;
  }

  if (match_many(s, "1>>")) {
    add_token(s, TK_STDOUT_REDIR_APPEND);
    return 0;
  }

  if (match_many(s, "1>")) {
    add_token(s, TK_STDOUT_REDIR);
    return 0;
  }

  // stderr redirects
  if (match_many(s, "2>>")) {
    add_token(s, TK_STDERR_REDIR_APPEND);
    return 0;
  }

  if (match_many(s, "2>")) {
    add_token(s, TK_STDERR_REDIR);
    return 0;
  }

  // stdout and stderr redirects
  if (match_many(s, "&>>")) {
    add_token(s, TK_STDOUT_ERR_REDIR_APPEND);
    return 0;
  }

  if (match_many(s, "&>")) {
    add_token(s, TK_STDOUT_ERR_REDIR);
    return 0;
  }

  if (match_many(s, "||")) {
    add_token(s, TK_LOGICAL_OR);
    return 0;
  }

  if (match(s, '|')) {
    add_token(s, TK_PIPE);
    return 0;
  }

  if (match(s, ';')) {
    add_token(s, TK_SEMI);
    return 0;
  }

  if (match_many(s, "&&")) {
    add_token(s, TK_LOGICAL_AND);
    return 0;
  }

  if (match(s, '&')) {
    add_token(s, TK_AMP);
    return 0;
  }

  if (match_many(s, "&&")) {
    add_token(s, TK_LOGICAL_AND);
    return 0;
  }

  if (match(s, '&')) {
    add_token(s, TK_AMP);
    return 0;
  }

  if (match_many(s, "**")) {
    add_token(s, TK_ARG_DOUBLESTAR);
    return 0;
  }

  if (match(s, '*')) {
    add_token(s, TK_ARG_WILDCARD);
    return 0;
  }

  if (match(s, '!')) {
    add_token(s, TK_MACRO);
    return 0;
  }

  if (match(s, '~')) {
    return tilde(s);
  }

  return 0;
}

TokenList lex(const Buffer *source) {
  LexState state = {.source = source, .current = 0, .start = 0};
  VECTOR_INIT(state.tokens);

  while (!is_at_end(&state)) {
    // We are at the beginning of the next lexeme.
    state.start = state.current;
    if (scan_token(&state)) {
      token_list_destroy(&state.tokens);
      return (TokenList){.length = 0, ._capacity = 0, .data = NULL};
    }
  }

  return state.tokens;
}
