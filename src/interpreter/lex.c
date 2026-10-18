#include "interpreter/lex.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "interpreter/token.h"
#include "lib/attrib.h"
#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/parse.h"
#include "lib/vector.h"

typedef struct {
  const Buffer *source;
  TokenList tokens;
  size_t start;
  size_t current;

  bool is_in_argument;
  Buffer current_word;
} LexState;

// if c is alpha numeric or an underscore
static bool is_ident(uint8_t c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

// if c is a letter or an underscore
static bool is_begin_ident(uint8_t c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// if cstr is a valid identifier
ATTRIB_UNUSED
static bool is_ident_cstr(const char *cstr) {
  if (!is_begin_ident((uint8_t)*cstr)) {
    return false;
  }

  cstr++;

  while (*cstr != '\0') {
    if (!is_ident((uint8_t)*cstr)) {
      return false;
    }

    cstr++;
  }

  return true;
}

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
    if (is_at_end(s) || s->source->char_ptr[s->current + i] != expected[i]) {
      return false;
    }
  }

  s->current += i;
  return true;
}

static void add_token(LexState *s, TokenKind kind) {
  VECTOR_PUSH(s->tokens, ((Token){.kind = kind}));
}

static void add_buffer_token(LexState *s, TokenKind kind, Buffer buffer) {
  assert(IS_BUFFER_TOKEN(kind));

  VECTOR_PUSH(s->tokens, ((Token){.kind = kind, .buffer = buffer}));
}

static void add_double_token(LexState *s, TokenKind kind, double number) {
  VECTOR_PUSH(s->tokens, ((Token){.kind = kind, .number = number}));
}

static int shell_expr_string(LexState *s) {
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
  add_buffer_token(s, TK_SE_STRING_LIT, value);
  return 0;
}

static int shell_expr_number(LexState *s) {
  while (isdigit(peek(s))) {
    advance(s);
  }

  // Look for a fractional part.
  if (peek(s) == '.' && isdigit(peek_next(s))) {
    // Consume the "."
    advance(s);

    while (isdigit(peek(s))) {
      advance(s);
    }
  }

  Buffer number_str =
      buffer_from_ptr(s->source->u8_ptr + s->start, s->current - s->start);

  add_double_token(
      s, TK_SE_NUMBER_LIT, parse_double(buffer_cstr(&number_str)).value
  );
  buffer_destroy(&number_str);

  return 0;
}

static int shell_expr_identifier(LexState *s) {
  while (is_ident(peek(s))) {
    advance(s);
  }

  Buffer identifier =
      buffer_from_ptr(s->source->u8_ptr + s->start, s->current - s->start);

  if (buffer_compare_cstr(&identifier, "null") == 0) {
    add_token(s, TK_SE_NULL_LIT);
    buffer_destroy(&identifier);
    return 0;
  }

  if (buffer_compare_cstr(&identifier, "true") == 0) {
    add_token(s, TK_SE_TRUE_LIT);
    buffer_destroy(&identifier);
    return 0;
  }

  if (buffer_compare_cstr(&identifier, "false") == 0) {
    add_token(s, TK_SE_FALSE_LIT);
    buffer_destroy(&identifier);
    return 0;
  }

  add_buffer_token(s, TK_SE_IDENTIFIER, identifier);

  return 0;
}

static int standalone_shell_expr(LexState *s) {
  while (!is_at_end(s)) {
    s->start = s->current;
    
      if (match(s, '(')) {
        add_token(s, TK_SE_O_PAREN);
        continue;
      }

      if (match(s, ')')) {
        add_token(s, TK_SE_C_PAREN);
        continue;
      }

      if (match(s, '+')) {
        add_token(s, TK_SE_ADD);
        continue;
      }

      if (match(s, '-')) {
        add_token(s, TK_SE_SUB);
        continue;
      }

      if (match(s, '*')) {
        if (match(s, '*')) {
          add_token(s, TK_SE_POW);
          continue;
        }
        add_token(s, TK_SE_MUL);
        continue;
      }

      if (match(s, '/')) {
        add_token(s, TK_SE_DIV);
        continue;
      }

      if (match(s, '%')) {
        add_token(s, TK_SE_MOD);
        continue;
      }

      if (match(s, '=')) {
        if (match(s, '=')) {
          add_token(s, TK_SE_EQ);
          continue;
        }

        error("shell expression: found stray `=`.\n");
        return -1;
      }

      if (match(s, '!')) {
        if (match(s, '=')) {
          add_token(s, TK_SE_NEQ);
          continue;
        }
        add_token(s, TK_SE_NOT);
        continue;
      }

      if (match(s, '>')) {
        if (match(s, '=')) {
          add_token(s, TK_SE_GTE);
          continue;
        }
        add_token(s, TK_SE_GT);
        continue;
      }

      if (match(s, '<')) {
        if (match(s, '=')) {
          add_token(s, TK_SE_LTE);
          continue;
        }
        add_token(s, TK_SE_LT);
        continue;
      }

      if (match(s, '?')) {
        add_token(s, TK_SE_QUESTION);
        continue;
      }

      if (match(s, ':')) {
        add_token(s, TK_SE_COLON);
        continue;
      }

      if (match(s, ',')) {
        add_token(s, TK_SE_COMMA);
        continue;
      }

      if (match(s, '"')) {
        if (shell_expr_string(s)) {
          return -1;
        }
        continue;
      }

      if (match(s, ' ')) {
        continue;
      }

      if (isdigit((int)peek(s))) {
        advance(s);
        if (shell_expr_number(s)) {
          return -1;
        }
        continue;
      }
      if (is_begin_ident(peek(s))) {
        advance(s);
        if (shell_expr_identifier(s)) {
          return -1;
        }
        continue;
      }

      // unrecognized character, stop lexing shell expr
      break;
  }

  return 0;
}

static int shell_expr(LexState *s) {
  add_token(s, TK_SE_BEGIN);
  if (standalone_shell_expr(s)) {
    return -1;
  }
  if (!match(s, '}')) {
    error_f("rash: expected closing ‘}’ character.\n");
    return -1;
  }
  add_token(s, TK_SE_END);
  return 0;
}

TokenList lex_shell_expr(const Buffer *source) {
  LexState state = {.source = source, .current = 0, .start = 0};

  VECTOR_INIT(state.tokens);

  
  if (standalone_shell_expr(&state)) {
    token_list_destroy(&state.tokens);
    return (TokenList){.length = 0, ._capacity = 0, .data = NULL};
  }

  if (!is_at_end(&state)) {
    error_f("shell expression: unrecognized character `%c`.\n", peek(&state));
    token_list_destroy(&state.tokens);
    return (TokenList){.length = 0, ._capacity = 0, .data = NULL};
  }

  return state.tokens;
}

// what is and isn't allowed in a username isn't set in stone. i'm limiting it
// to lower and upper case ASCII letters, digits, period, underscore, and hyphen
// see here: https://systemd.io/USER_NAMES/
static bool is_username_allowed(uint8_t c) {
  return (
      (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') || (c == '.') || (c == '_') || (c == '-')
  );
}

static int tilde(LexState *s) {
  while (is_username_allowed(peek(s))) {
    advance(s);
  }

  Buffer username = buffer_slice(s->source, s->start + 1, s->current);

  add_buffer_token(s, TK_ARG_TILDE, username);
  return 0;
}

static int subshell(LexState *s) {
  while (peek(s) != ')' && !is_at_end(s)) {
    advance(s);
  }

  if (is_at_end(s)) {
    error_f("rash: expected closing ‘)’ character.\n");
    return -1;
  }

  // closing ')'
  advance(s);

  Buffer cmd = buffer_slice(s->source, s->start + 2, s->current - 1);

  if (cmd.length == 0) {
    error_f("rash: subshell cannot be empty.\n");
    buffer_destroy(&cmd);
    return -1;
  }

  add_buffer_token(s, TK_ARG_SUBSHELL, cmd);
  return 0;
}

static bool is_env_allowed(uint8_t c) {
  return (
      (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') || (c == '.') || (c == '_') || (c == '-')
  );
}

static int environment(LexState *s) {
  Buffer env;

  if (match(s, '{')) {
    while (peek(s) != '}' && !is_at_end(s)) {
      advance(s);
    }

    if (is_at_end(s)) {
      error_f("rash: expected closing ‘}’ character.\n");
      return -1;
    }

    // closing '}'
    advance(s);

    env = buffer_slice(
        s->source,
        // +2 for the '$' and '{'
        s->start + 2,
        // -1 for the '}'
        s->current - 1
    );
  } else {
    while (is_env_allowed(peek(s))) {
      advance(s);
    }

    env = buffer_slice(
        s->source,
        // +1 for the '$'
        s->start + 1,
        // -1 for the '}'
        s->current
    );
  }

  if (env.length == 0) {
    error_f("rash: cannot expand empty enviroment variable.\n");
    buffer_destroy(&env);
    return -1;
  }

  add_buffer_token(s, TK_ARG_ENV, env);
  return 0;
}

static int dollar(LexState *s) {
  if (match(s, '(')) {
    return subshell(s);
  }

  return environment(s);
}

static void add_string_token_if_needed(LexState *s) {
  if (s->current_word.length > 0) {
    add_buffer_token(s, TK_ARG_STRING, s->current_word);

    s->current_word = buffer_create(0);
  }
}

static int double_quote(LexState *s) {
  while (1) {
    // we need to set this so that dollar, and shell_expr know where they are
    s->start = s->current;

    if (match(s, '$')) {
      add_string_token_if_needed(s);
      return dollar(s);
    }

    if (match(s, '{')) {
      add_string_token_if_needed(s);
      return shell_expr(s);
    }

    if (match(s, '"')) {
      return 0;
    }

    if (is_at_end(s)) {
      error_f("rash: Expected closing ‘\"’ character.\n");
      return -1;
    }

    buffer_append(&s->current_word, advance(s));
  }
}

static int single_quote(LexState *s) {
  while (1) {
    if (match(s, '\'')) {
      return 0;
    }

    if (is_at_end(s)) {
      error_f("rash: Expected closing ‘\"’ character.\n");
      return -1;
    }

    buffer_append(&s->current_word, advance(s));
  }
}

static void end_argument_if_needed(LexState *s) {
  if (s->is_in_argument) {
    add_string_token_if_needed(s);
    add_token(s, TK_ARG_END);
    s->is_in_argument = false;
  }
}

static int scan_token(LexState *s) {
  // ------- stdin redirects -------
  if (match_many(s, "<<<")) {
    end_argument_if_needed(s);
    add_token(s, TK_STDIN_REDIR_STRING);
    return 0;
  }

  if (match(s, '<')) {
    end_argument_if_needed(s);
    add_token(s, TK_STDIN_REDIR);
    return 0;
  }

  // ------- stdout redirects -------
  if (match_many(s, ">>")) {
    end_argument_if_needed(s);
    add_token(s, TK_STDOUT_REDIR_APPEND);
    return 0;
  }

  if (match(s, '>')) {
    end_argument_if_needed(s);
    add_token(s, TK_STDOUT_REDIR);
    return 0;
  }

  if (match_many(s, "1>>")) {
    end_argument_if_needed(s);
    add_token(s, TK_STDOUT_REDIR_APPEND);
    return 0;
  }

  if (match_many(s, "1>")) {
    end_argument_if_needed(s);
    add_token(s, TK_STDOUT_REDIR);
    return 0;
  }

  // ------- stderr redirects -------
  if (match_many(s, "2>>")) {
    end_argument_if_needed(s);
    add_token(s, TK_STDERR_REDIR_APPEND);
    return 0;
  }

  if (match_many(s, "2>")) {
    end_argument_if_needed(s);
    add_token(s, TK_STDERR_REDIR);
    return 0;
  }

  // ------- stdout and stderr redirects -------
  if (match_many(s, "&>>")) {
    end_argument_if_needed(s);
    add_token(s, TK_STDOUT_ERR_REDIR_APPEND);
    return 0;
  }

  if (match_many(s, "&>")) {
    end_argument_if_needed(s);
    add_token(s, TK_STDOUT_ERR_REDIR);
    return 0;
  }

  // ------- command separators -------

  if (match_many(s, "||")) {
    end_argument_if_needed(s);
    add_token(s, TK_LOGICAL_OR);
    return 0;
  }

  if (match(s, '|')) {
    end_argument_if_needed(s);
    add_token(s, TK_PIPE);
    return 0;
  }

  if (match(s, ';')) {
    end_argument_if_needed(s);
    add_token(s, TK_SEMI);
    return 0;
  }

  if (match_many(s, "&&")) {
    end_argument_if_needed(s);
    add_token(s, TK_LOGICAL_AND);
    return 0;
  }

  if (match(s, '&')) {
    end_argument_if_needed(s);
    add_token(s, TK_AMP);
    return 0;
  }

  if (match_many(s, "&&")) {
    end_argument_if_needed(s);
    add_token(s, TK_LOGICAL_AND);
    return 0;
  }

  // ------- whitespace -------

  if (match(s, ' ')) {
    end_argument_if_needed(s);
    return 0;
  }

  // ------- comments -------

  if (match_many(s, "# ")) {
    end_argument_if_needed(s);
    s->current = s->source->length;
    return 0;
  }

  // ------- argument things -------

  if (!s->is_in_argument) {
    s->is_in_argument = true;
    add_token(s, TK_ARG_BEGIN);
  }

  if (match_many(s, "**")) {
    add_string_token_if_needed(s);
    add_token(s, TK_ARG_DOUBLESTAR);
    return 0;
  }

  if (match(s, '*')) {
    add_string_token_if_needed(s);
    add_token(s, TK_ARG_WILDCARD);
    return 0;
  }

  if (match(s, '~')) {
    add_string_token_if_needed(s);
    return tilde(s);
  }

  if (match(s, '$')) {
    add_string_token_if_needed(s);
    return dollar(s);
  }

  if (match(s, '{')) {
    add_string_token_if_needed(s);
    return shell_expr(s);
  }

  if (match(s, '\'')) {
    add_string_token_if_needed(s);
    return single_quote(s);
  }

  if (match(s, '"')) {
    return double_quote(s);
  }

  buffer_append(&s->current_word, advance(s));

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
      buffer_destroy(&state.current_word);
      return (TokenList){.length = 0, ._capacity = 0, .data = NULL};
    }
  }

  end_argument_if_needed(&state);

  return state.tokens;
}
