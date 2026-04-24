#include "lex.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lib/buffer.h"
#include "lib/error.h"
#include "lib/string.h"
#include "lib/vector.h"

void free_arg_part(ArgumentPart *part);
void free_token(Token *token);

typedef struct {
  size_t start;
  size_t current;
  TokenList tokens;
  Argument argument;
  String arg_buffer;
  const Buffer *source;
} LexerState;

uint8_t advance(LexerState *state) {
  return state->source->data[state->current++];
}

void add_arg_part(LexerState *state, ArgumentPartType type) {
  if (state->arg_buffer.length != 0) {
    VECTOR_PUSH(state->arg_buffer, '\0');

    VECTOR_PUSH(
        state->argument,
        ((ArgumentPart){.type = STRING, .data = state->arg_buffer.data}));

    VECTOR_INIT_EMPTY(state->arg_buffer);
  }

  VECTOR_PUSH(state->argument, (ArgumentPart){.type = type});
}

void add_arg_part_data(LexerState *state, ArgumentPartType type,
                       char *data // NOLINT(readability-non-const-parameter)
) {
  if (state->arg_buffer.length != 0) {
    VECTOR_PUSH(state->arg_buffer, '\0');

    VECTOR_PUSH(
        state->argument,
        ((ArgumentPart){.type = STRING, .data = state->arg_buffer.data}));

    VECTOR_INIT_EMPTY(state->arg_buffer);
  }

  VECTOR_PUSH(state->argument, ((ArgumentPart){.type = type, .data = data}));
}

void flush_argument(LexerState *state) {
  if (state->arg_buffer.length != 0 || state->argument.length != 0) {
    add_arg_part(state, END_ARG);

    VECTOR_PUSH(state->tokens,
                ((Token){.type = ARGUMENT, .data = state->argument.data}));

    VECTOR_INIT_EMPTY(state->argument);
  }
}

void add_token(LexerState *state, TokenType type) {
  flush_argument(state);
  VECTOR_PUSH(state->tokens, (Token){.type = type});
}

void add_token_data(LexerState *state, TokenType type, void *data) {
  flush_argument(state);
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

void decrement(LexerState *state) {
  assert(state->current != 0);
  state->current--;
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

  add_arg_part_data(state, SUBSHELL, subshell_cmd);

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
      VECTOR_PUSH(state->arg_buffer, '$');
      return 0;
    }
  }

  char *env_name = malloc(env_len + 1);
  memcpy(env_name, state->source->data + start, env_len);
  env_name[env_len] = '\0';

  add_arg_part_data(state, ENV_EXPANSION, env_name);
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

  add_arg_part_data(state, VAR_EXPANSION, var_name);
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

  add_arg_part_data(state, TILDE, user);

  return 0;
}

int parse_double_quote(LexerState *state) {
  while (!is_at_end(state)) {
    uint8_t c = advance(state);

    if (c == '"') {
      return 0;
    }

    if (c == '\\') {
      if (match(state, '"')) {
        VECTOR_PUSH(state->arg_buffer, '"');
        continue;
      }

      if (match(state, '$')) {
        VECTOR_PUSH(state->arg_buffer, '$');
        continue;
      }

      if (match(state, '{')) {
        VECTOR_PUSH(state->arg_buffer, '{');
        continue;
      }
    }

    if (c == '$') {
      if (match(state, '(')) {
        if (parse_subshell(state)) {
          return -1;
        }
      }
      parse_env(state);
      continue;
    }

    if (c == '{') {
      if (parse_var(state)) {
        return -1;
      }
      continue;
    }

    VECTOR_PUSH(state->arg_buffer, (char)c);
  }

  error_f("rash: Expected closing ‘\"’ character.\n");

  return -1;
}

int parse_single_quote(LexerState *state) {
  while (!is_at_end(state)) {
    uint8_t c = advance(state);

    if (c == '\\' && match(state, '\'')) {
      VECTOR_PUSH(state->arg_buffer, '\'');
      continue;
    }

    if (c == '\'') {
      return 0;
    }

    VECTOR_PUSH(state->arg_buffer, (char)c);
  }

  error_f("rash: Expected closing ‘'’ character.\n");

  return -1;
}

int scan_token(LexerState *state) {
  uint8_t c = advance(state);
  switch (c) {
  case '<':
    if (match(state, '<')) {
      if (match(state, '<')) {
        add_token(state, STDIN_REDIR_STRING);
      }

      string_append(&state->arg_buffer, "<<");
      break;
    }
    add_token(state, STDIN_REDIR);
    break;

  case '>':
    add_token(state, match(state, '>') ? STDOUT_REDIR_APPEND : STDOUT_REDIR);
    break;

  case '2':
    if (match(state, '>')) {
      add_token(state, match(state, '>') ? STDERR_REDIR_APPEND : STDERR_REDIR);
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

  case '"':
    if (parse_double_quote(state)) {
      goto error;
    }
    break;

  case '\'':
    if (parse_single_quote(state)) {
      goto error;
    }
    break;

  case '$':
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

  case '{':
    if (parse_var(state)) {
      goto error;
    }
    break;

  case '~':
    if (parse_tilde(state)) {
      goto error;
    }
    break;

  case '*':
    add_arg_part(state, GLOB_WILDCARD);
    break;

  case '\\': {
    char new = (char)advance(state);
    if (is_at_end(state)) {
      error_f("rash: Expected character after ‘\\’.\n");
      goto error;
    }

    VECTOR_PUSH(state->arg_buffer, new);
    break;
  }

  case ' ':
  case '\n':
  case '\r':
  case '\t':
    flush_argument(state);
    break;

  default:
    VECTOR_PUSH(state->arg_buffer, (char)c);
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

  VECTOR_DESTROY(state.arg_buffer);

  return state.tokens.data;

error:
  for (size_t i = 0; i < state.tokens.length; i++) {
    free_token(state.tokens.data + i);
  }

  for (size_t i = 0; i < state.argument.length; i++) {
    free_arg_part(state.argument.data + i);
  }

  VECTOR_DESTROY(state.arg_buffer);
  VECTOR_DESTROY(state.tokens);
  return NULL;
}

void free_tokens(Token *tokens) {
  for (size_t i = 0; tokens[i].type != END; i++) {
    free_token(tokens + i);
  }

  free(tokens);
}

void free_token(Token *token) {
  if (token->type == ARGUMENT) {
    ArgumentPart *part = token->data;

    for (size_t j = 0; part[j].type != END_ARG; j++) {
      free_arg_part(part + j);
    }

    free(part);
  }
}

void free_arg_part(ArgumentPart *part) {
  if (
    part->type == STRING        || 
    part->type == ENV_EXPANSION ||
    part->type == SUBSHELL      || 
    part->type == VAR_EXPANSION ||
    part->type == TILDE
  ) {
    free(part->data);
  }
}
