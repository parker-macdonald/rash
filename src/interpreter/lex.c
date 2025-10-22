#include "lex.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../vector.h"

const char *const TOKEN_NAMES[] = {
    "STRING",
    "STDIN_REDIR",
    "STDIN_REDIR_STRING",
    "STDOUT_REDIR",
    "STDOUT_REDIR_APPEND",
    "STDERR_REDIR",
    "STDERR_REDIR_APPEND",
    "PIPE",
    "GLOB",
    "SEMI",
    "LOGICAL_AND",
    "LOGICAL_OR",
    "AMP",
    "END"
};

enum lexer_state {
  DEFAULT,
  WHITESPACE,
  SINGLE_QUOTE,
  DOUBLE_QUOTE,
  SINGLE_LITERAL
};

#define ADD_NONSTR_TOKEN(token_type)                                           \
  do {                                                                         \
    if (buffer.length != 0) {                                                  \
      VECTOR_PUSH(buffer, '\0');                                               \
      VECTOR_PUSH(tokens, ((token_t){.type = STRING, .data = buffer.data}));   \
      VECTOR_INIT(buffer);                                                     \
    }                                                                          \
    VECTOR_PUSH(tokens, (token_t){.type = token_type});                        \
  } while (0)

#define ADD_ENV_TO_VECTOR(env_name, vector)                                    \
  do {                                                                         \
    uint8_t *env_value = (uint8_t *)getenv(env_name);                          \
                                                                               \
    if (env_value != NULL) {                                                   \
      for (size_t j = 0; env_value[j] != '\0'; j++) {                          \
        VECTOR_PUSH(vector, env_value[j]);                                     \
      }                                                                        \
    }                                                                          \
  } while (0)

token_t *lex(const uint8_t *source) {
  VECTOR(token_t) tokens;
  VECTOR_INIT(tokens);

  VECTOR(uint8_t) buffer;
  VECTOR_INIT(buffer);

  enum lexer_state state = WHITESPACE;

  for (size_t i = 0; source[i] != '\0'; i++) {
    const uint8_t curr = source[i];

    switch (state) {
      case DEFAULT:
        if (curr == '"') {
          state = DOUBLE_QUOTE;
          break;
        }

        if (curr == '\'') {
          state = SINGLE_QUOTE;
          break;
        }

        if (curr == '\\') {
          state = SINGLE_LITERAL;
          break;
        }

        if (isspace((int)curr)) {
          state = WHITESPACE;
          if (buffer.length != 0) {
            VECTOR_PUSH(buffer, '\0');
            VECTOR_PUSH(
                tokens, ((token_t){.type = STRING, .data = buffer.data})
            );
            VECTOR_INIT(buffer);
          }

          break;
        }

        // stdin redirection
        if (curr == '<') {
          if (source[i + 1] == '<' && source[i + 2] == '<') {
            ADD_NONSTR_TOKEN(STDIN_REDIR_STRING);
            i += 2;
            break;
          } else {
            ADD_NONSTR_TOKEN(STDIN_REDIR);
            break;
          }
        }

        // stdout redirection
        if (curr == '>') {
          if (source[i + 1] == '>') {
            ADD_NONSTR_TOKEN(STDOUT_REDIR_APPEND);
            i++;
            break;
          }
          ADD_NONSTR_TOKEN(STDOUT_REDIR);
          break;
        }

        if (curr == '2') {
          if (source[i + 1] == '>') {
            if (source[i + 2] == '>') {
              ADD_NONSTR_TOKEN(STDERR_REDIR_APPEND);
              i += 2;
              break;
            }
            ADD_NONSTR_TOKEN(STDERR_REDIR);
            i++;
            continue;
          }
        }

        if (curr == '|') {
          if (source[i + 1] == '|') {
            ADD_NONSTR_TOKEN(LOGICAL_OR);
            i++;
            break;
          }
          ADD_NONSTR_TOKEN(PIPE);
          break;
        }

        if (curr == ';') {
          ADD_NONSTR_TOKEN(SEMI);
          break;
        }

        if (curr == '&') {
          if (source[i + 1] == '&') {
            ADD_NONSTR_TOKEN(LOGICAL_AND);
            i++;
            break;
          }
          ADD_NONSTR_TOKEN(AMP);
          break;
        }

        // crazy logic for enviroment variables
        if (curr == '$') {
          if (source[i + 1] == '\0') {
            VECTOR_PUSH(buffer, '$');
            break;
          }

          // special variables
          i++;
          if (isdigit((int)source[i]) || source[i] == '?') {
            char env_name[2];
            env_name[0] = (char)source[i];
            env_name[1] = '\0';

            ADD_ENV_TO_VECTOR(env_name, buffer);
            break;
          }

          size_t env_len = 0;
          const uint8_t *env_start = source + i;

          if (source[i] == '{') {
            i++;
            env_start++;

            for (;;) {
              if (source[i] == '}') {
                break;
              }

              if (source[i] == '\0') {
                fprintf(stderr, "rash: expected closing ‘}’ character.\n");
                goto error;
              }

              i++;
              env_len++;
            }

            if (env_len == 0) {
              fprintf(
                  stderr, "rash: cannot expand empty enviroment variable.\n"
              );
              goto error;
            }

            char *env_name = malloc(env_len + 1);
            memcpy(env_name, env_start, env_len);
            env_name[env_len] = '\0';

            ADD_ENV_TO_VECTOR(env_name, buffer);
            free(env_name);
            break;
          }

          for (;;) {
            if (!isalnum((int)source[i]) || source[i] == '\0') {
              i--;
              break;
            }

            i++;
            env_len++;
          }

          assert(env_len != 0);

          char *env_name = malloc(env_len + 1);
          memcpy(env_name, env_start, env_len);
          env_name[env_len] = '\0';

          ADD_ENV_TO_VECTOR(env_name, buffer);
          free(env_name);
          break;
        }

        VECTOR_PUSH(buffer, curr);
        break;

      case WHITESPACE:
        if (!isspace((int)curr)) {
          state = DEFAULT;
          i--;
        }
        break;

      case DOUBLE_QUOTE:
        if (curr == '"') {
          state = DEFAULT;
          break;
        }

        VECTOR_PUSH(buffer, curr);
        break;

      case SINGLE_QUOTE:
        if (curr == '\'') {
          state = DEFAULT;
          break;
        }

        VECTOR_PUSH(buffer, curr);
        break;

      case SINGLE_LITERAL:
        VECTOR_PUSH(buffer, curr);
        state = DEFAULT;
        break;
    }
  }

  switch (state) {
    case SINGLE_LITERAL:
      fprintf(stderr, "rash: Expected character after ‘\\’.\n");
      goto error;
    case SINGLE_QUOTE:
      fprintf(stderr, "rash: Expected closing ‘'’ character.\n");
      goto error;
    case DOUBLE_QUOTE:
      fprintf(stderr, "rash: Expected closing ‘\"’ character.\n");
      goto error;
    default:
      break;
  }

  if (buffer.length != 0) {
    VECTOR_PUSH(buffer, '\0');
    VECTOR_PUSH(tokens, ((token_t){.type = STRING, .data = buffer.data}));
  } else {
    VECTOR_DESTROY(buffer);
  }

  VECTOR_PUSH(tokens, (token_t){.type = END});

  return tokens.data;

error:
  VECTOR_DESTROY(buffer);

  for (size_t i = 0; i < tokens.length; i++) {
    if (tokens.data[i].type == STRING || tokens.data[i].type == GLOB) {
      free(tokens.data[i].data);
    }
  }

  VECTOR_DESTROY(tokens);

  return NULL;
}

void free_tokens(token_t **tokens) {
  for (size_t i = 0; (*tokens)[i].type != END; i++) {
    if ((*tokens)[i].type == STRING || (*tokens)[i].type == GLOB) {
      free((*tokens)[i].data);
    }
  }

  free(*tokens);
}
