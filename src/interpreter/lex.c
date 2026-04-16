#include "lex.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lib/error.h"
#include "lib/string.h"
#include "lib/vector.h"

typedef struct {
  size_t start;
  size_t current;
  TokenList tokens;
  String current_arg;
  const Buffer *source;
} LexerState;

typedef enum {
  DEFAULT,
  SINGLE_QUOTE,
  DOUBLE_QUOTE
} ArgumentState;

uint8_t advance(LexerState *state) {
  return state->source->data[state->current++];
}

void add_token(LexerState *state, TokenType type) {
  VECTOR_PUSH(state->tokens, (Token){.type = type});
}

// data cannot be marked as const, this lint warning is a false positive
void add_token_data(
    LexerState *state,
    TokenType type,
    char *data // NOLINT(readability-non-const-parameter)
) {
  VECTOR_PUSH(state->tokens, ((Token){.type = type, .data = data}));
}

bool is_at_end(LexerState *state) {
  return state->current >= state->source->length;
}

bool match(LexerState *state, uint8_t expected) {
  if (is_at_end(state)) {
    return false;
  }

  if (state->source->data[state->current] != expected) {
    return false;
  }

  state->current++;
  return true;
}

uint8_t peek(LexerState *state) {
  if (is_at_end(state)) {
    return '\0';
  }
  return state->source->data[state->current];
}

void flush_current_arg(LexerState *state) {
  if (state->current_arg.length != 0) {
    VECTOR_PUSH(state->current_arg, '\0');

    add_token_data(state, STRING, state->current_arg.data);

    VECTOR_INIT_EMPTY(state->current_arg);
  }
}

int parse_subshell(LexerState *state) {
  size_t start = state->current;

  while (peek(state) != ')' && !is_at_end(state)) {
    advance(state);
  }

  if (is_at_end(state)) {
    error_f("rash: expected closing ‘)’ character.\n");
    return -1;
  }

  if (start == state->current) {
    error_f("rash: subshell cannot be empty.\n");
    return -1;
  }

  size_t subshell_len = state->current - start;
  char *subshell_cmd = malloc(subshell_len + 1);
  memcpy(subshell_cmd, state->source->data + start, subshell_len);
  subshell_cmd[subshell_len] = '\0';

  flush_current_arg(state);
  add_token_data(state, SUBSHELL, subshell_cmd);

  // closing )
  advance(state);

  return 0;
}

int parse_env(LexerState *state) {
  size_t start;
  size_t env_len;

  if (match(state, '{')) {
    start = state->current;

    while (peek(state) != '}' && !is_at_end(state)) {
      advance(state);
    }

    if (is_at_end(state)) {
      error_f("rash: expected closing ‘}’ character.\n");
      return -1;
    }

    if (start == state->current) {
      error_f("rash: cannot expand empty enviroment variable.\n");
      return -1;
    }

    env_len = state->current - start;

    // closeing }
    advance(state);
  } else {
    start = state->current;

    while (1) {
      if (!isalnum((int)peek(state)) && peek(state) != '_') {
        break;
      }

      if (is_at_end(state)) {
        break;
      }

      advance(state);
    }

    env_len = state->current - start;

    if (start == state->current) {
      VECTOR_PUSH(state->current_arg, '$');
      return 0;
    }
  }

  
  char *env_name = malloc(env_len + 1);
  memcpy(env_name, state->source->data + start, env_len);
  env_name[env_len] = '\0';

  flush_current_arg(state);
  add_token_data(state, ENV_EXPANSION, env_name);
  return 0;
}

int parse_var(LexerState *state) {
  size_t start = state->current;

  while (peek(state) != '}' && !is_at_end(state)) {
    advance(state);
  }

  if (is_at_end(state)) {
    error_f("rash: expected closing ‘}’ character.\n");
    return 1;
  }

  if (start == state->current) {
    error_f("rash: cannot expand empty shell variable.\n");
    return 1;
  }

  size_t var_len = state->current - start;
  char *var_name = malloc(var_len + 1);
  memcpy(var_name, state->source->data + start, var_len);
  var_name[var_len] = '\0';

  flush_current_arg(state);
  add_token_data(state, VAR_EXPANSION, var_name);
  // closing }
  advance(state);

  return 0;
}

int parse_tilde(LexerState *state) {
  size_t start = state->current;

  while (1) {
    if (peek(state) == '/' || isspace((int)peek(state)) || is_at_end(state)) {
      break;
    }

    advance(state);
  }

  size_t user_len = state->current - start;
  char *user = malloc(user_len + 1);
  memcpy(user, state->source->data + start, user_len);
  user[user_len] = '\0';

  add_token_data(state, TILDE, user);
  advance(state);
  return 0;
}

int parse_argument(LexerState *state) {
  ArgumentState arg_state = DEFAULT;

  if (match(state, '~')) {
    if (parse_tilde(state)) {
      goto error;
    }
  }

  while (1) {
    if (is_at_end(state)) {
      break;
    }

    uint8_t c = advance(state);

    switch (arg_state) {
      case DEFAULT:
        if (c == '"') {
          arg_state = DOUBLE_QUOTE;
          break;
        }

        if (c == '\'') {
          arg_state = SINGLE_QUOTE;
          break;
        }

        if (c == '$') {
          if (match(state, '(')) {
            if (parse_subshell(state)) {
              goto error;
            }
            break;
          }
          if (parse_env(state)) {
            goto error;
          }
          break;
        }

        if (c == '{') {
          if (parse_var(state)) {
            goto error;
          }
          break;
        }

        if (c == '*') {
          flush_current_arg(state);
          add_token(state, GLOB_WILDCARD);
          break;
        }

        if (c == '\\') {
          char new = (char)advance(state);
          if (is_at_end(state)) {
            error_f("rash: Expected character after ‘\\’.\n");
            goto error;
          }

          VECTOR_PUSH(state->current_arg, new);
          break;
        }

        if (c == ' ') {
          goto leave;
        }

        VECTOR_PUSH(state->current_arg, (char)c);
        break;

      case DOUBLE_QUOTE:
        if (c == '"') {
          arg_state = DEFAULT;
          break;
        }

        if (c == '\\' && match(state, '"')) {
          VECTOR_PUSH(state->current_arg, '"');
          break;
        }

        if (c == '$') {
          if (match(state, '(')) {
            if (parse_subshell(state)) {
              goto error;
            }
          }
          parse_env(state);
          break;
        }

        if (c == '{') {
          if (parse_var(state)) {
            goto error;
          }
          break;
        }

        VECTOR_PUSH(state->current_arg, (char)c);
        break;

      case SINGLE_QUOTE:
        if (c == '\\' && match(state, '\'')) {
          VECTOR_PUSH(state->current_arg, '\'');
          break;
        }

        if (c == '\'') {
          arg_state = DEFAULT;
          break;
        }

        VECTOR_PUSH(state->current_arg, (char)c);
        break;
    }
  }

leave:
  switch (arg_state) {
    case SINGLE_QUOTE:
      error_f("rash: Expected closing ‘'’ character.\n");
      goto error;
    case DOUBLE_QUOTE:
      error_f("rash: Expected closing ‘\"’ character.\n");
      goto error;
    default:
      break;
  }

  flush_current_arg(state);

  add_token(state, END_ARG);

  return 0;
error:
  return -1;
}

int scan_token(LexerState *state) {
  uint8_t c = advance(state);
  switch (c) {
    case '<':
      // match has side effects, so it makes sense to have redundant expressions
      if (match(state, '<') && // NOLINT(misc-redundant-expression)
          match(state, '<')) {
        add_token(state, STDIN_REDIR_STRING);
        break;
      }
      add_token(state, STDIN_REDIR);
      break;
    case '>':
      add_token(state, match(state, '>') ? STDOUT_REDIR_APPEND : STDOUT_REDIR);
      break;
    case '2':
      if (match(state, '>')) {
        add_token(
            state, match(state, '>') ? STDERR_REDIR_APPEND : STDERR_REDIR
        );
      }
      break;
    case '|':
      add_token(state, match(state, '|') ? LOGICAL_OR : PIPE);
      break;
    case '&':
      add_token(state, match(state, '&') ? LOGICAL_AND : AMP);
      break;
    case ';':
      add_token(state, SEMI);
      break;
    case ' ':
    case '\n':
    case '\r':
    case '\t':
      break;
    default:
      state->current--;
      if (parse_argument(state)) {
        goto error;
      }
      break;
  }

  return 0;

error:
  return -1;
}

Token *lex(const Buffer *source) {
  LexerState state = {0};
  state.source = source;

  while (!is_at_end(&state)) {
    if (scan_token(&state)) {
      goto error;
    }
  }

  add_token(&state, END);

  VECTOR_DESTROY(state.current_arg);

  return state.tokens.data;

error:
  VECTOR_DESTROY(state.current_arg);

  for (size_t i = 0; i < state.tokens.length; i++) {
    if (state.tokens.data[i].type == STRING ||
        state.tokens.data[i].type == ENV_EXPANSION ||
        state.tokens.data[i].type == VAR_EXPANSION ||
        state.tokens.data[i].type == TILDE ||
        state.tokens.data[i].type == SUBSHELL) {
      free(state.tokens.data[i].data);
    }
  }

  VECTOR_DESTROY(state.tokens);
  return NULL;
}

void free_tokens(Token **tokens) {
  for (size_t i = 0; (*tokens)[i].type != END; i++) {
    if ((*tokens)[i].type == STRING || (*tokens)[i].type == ENV_EXPANSION ||
        (*tokens)[i].type == VAR_EXPANSION || (*tokens)[i].type == TILDE ||
        (*tokens)[i].type == SUBSHELL) {
      free((*tokens)[i].data);
    }
  }

  free(*tokens);
}
