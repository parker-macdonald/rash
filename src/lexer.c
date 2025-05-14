#include "lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"

enum lexer_state {
  DEFAULT,
  WHITESPACE,
  SINGLE_QUOTE,
  DOUBLE_QUOTE,
  VAR_EXPANSION,
  SINGLE_LITERAL
};

char **get_tokens_from_line(const uint8_t *const line) {
  VECTOR(uint8_t) buffer;
  VECTOR_INIT(buffer);

  VECTOR(size_t) tokens;
  VECTOR_INIT(tokens);

  size_t env_start = 0;
  size_t token_start = 0;

  enum lexer_state state = WHITESPACE;
  enum lexer_state prev_state = WHITESPACE;

  for (size_t i = 0; line[i] != '\0'; i++) {
    const uint8_t curr = line[i];

    switch (state) {
      case DEFAULT:
        if (isspace((int)curr)) {
          prev_state = state;
          state = WHITESPACE;

          if (token_start != i) {
            VECTOR_PUSH(buffer, '\0');
          }
          break;
        }

        if (curr == '$') {
          prev_state = state;
          state = VAR_EXPANSION;
          env_start = i;
          break;
        }

        if (curr == '"') {
          prev_state = state;
          state = DOUBLE_QUOTE;
          break;
        }

        if (curr == '\'') {
          prev_state = state;
          state = SINGLE_QUOTE;
          break;
        }

        if (curr == '\\') {
          prev_state = state;
          state = SINGLE_LITERAL;
          break;
        }

        VECTOR_PUSH(buffer, curr);
        break;

      case WHITESPACE:
        if (!isspace((int)curr)) {
          prev_state = state;
          state = DEFAULT;
          token_start = i--;
          VECTOR_PUSH(tokens, buffer.length);
        }
        break;

      case DOUBLE_QUOTE:
        if (curr == '$') {
          prev_state = state;
          state = VAR_EXPANSION;
          env_start = i;
          break;
        }

        if (curr == '"') {
          prev_state = state;
          state = DEFAULT;
          break;
        }

        VECTOR_PUSH(buffer, curr);
        break;

      case SINGLE_QUOTE:
        if (curr == '\'') {
          prev_state = state;
          state = DEFAULT;
          break;
        }

        VECTOR_PUSH(buffer, curr);
        break;

      case SINGLE_LITERAL:
        VECTOR_PUSH(buffer, curr);
        prev_state = state;
        state = DEFAULT;
        break;

      case VAR_EXPANSION:
        if (isdigit((int)curr) || curr == '?') {
          char env_name[2];
          env_name[0] = (char)curr;
          env_name[1] = '\0';

          const uint8_t *env_value = (uint8_t *)getenv(env_name);

          if (env_value != NULL) {
            for (size_t j = 0; env_value[j] != '\0'; j++) {
              VECTOR_PUSH(buffer, env_value[j]);
            }
          }

          state = prev_state;
          prev_state = VAR_EXPANSION;
          break;
        }

        if (!isalnum((int)curr) && curr != '_') {
          if (i == env_start + 1) {
            state = DEFAULT;
            VECTOR_PUSH(buffer, '$');
            i--;
            break;
          }

          const size_t env_len = i - env_start - 1;
          char *env_name = malloc(env_len + 1);
          strncpy(env_name, (const char *)&line[env_start + 1], env_len);
          env_name[env_len] = '\0';

          const uint8_t *env_value = (uint8_t *)getenv(env_name);
          free(env_name);

          free(env_name);

          if (env_value != NULL) {
            for (size_t j = 0; env_value[j] != '\0'; j++) {
              VECTOR_PUSH(buffer, env_value[j]);
            }
          }

          i--;
          state = prev_state;
          prev_state = VAR_EXPANSION;
          break;
        }

        break;
    }
  }

  switch (state) {
    case DEFAULT:
    case WHITESPACE:
    case VAR_EXPANSION:
      break;
    case SINGLE_LITERAL:
      fprintf(stderr, "Expected character after ‘\\’.\n");
      VECTOR_DESTROY(tokens);
      VECTOR_DESTROY(buffer);
      return NULL;
    case SINGLE_QUOTE:
      fprintf(stderr, "Expected closing ‘'’ character.\n");
      VECTOR_DESTROY(tokens);
      VECTOR_DESTROY(buffer);
      return NULL;
    case DOUBLE_QUOTE:
      fprintf(stderr, "Expected closing ‘\"’ character.\n");
      VECTOR_DESTROY(tokens);
      VECTOR_DESTROY(buffer);
      return NULL;
  }

  VECTOR_PUSH(buffer, '\0');

  for (size_t i = 0; i < tokens.length; i++) {
    tokens.data[i] += (size_t)buffer.data;
  }

  VECTOR_PUSH(tokens, 0x0);

  return (char **)tokens.data;
}
