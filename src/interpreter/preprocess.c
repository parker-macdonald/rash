#include "preprocess.h"

#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../shell_vars.h"
#include "../vector.h"

typedef VECTOR(uint8_t) buffer_t;

enum pp_state {
  DEFAULT,
  WHITESPACE,
  SINGLE_QUOTE,
  DOUBLE_QUOTE,
  SINGLE_LITERAL
};

void insert_string(buffer_t *buffer, const char *str) {
  bool needs_quoting = false;

  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] == '"') {
      needs_quoting = true;
      break;
    }

    if (str[i] == '\'') {
      needs_quoting = true;
      break;
    }

    if (str[i] == '\\') {
      needs_quoting = true;
      break;
    }

    if (isspace((int)str[i])) {
      needs_quoting = true;
      break;
    }

    // stdin redirection
    if (str[i] == '<') {
      if (str[i + 1] == '<' && str[i + 2] == '<') {
        needs_quoting = true;
        break;
      } else {
        needs_quoting = true;
        break;
      }
    }

    // stdout redirection
    if (str[i] == '>') {
      if (str[i + 1] == '>') {
        needs_quoting = true;
        break;
      }
      needs_quoting = true;
      break;
    }

    if (str[i] == '2') {
      if (str[i + 1] == '>') {
        if (str[i + 2] == '>') {
          needs_quoting = true;
          i += 2;
          break;
        }
        needs_quoting = true;
        break;
      }
    }

    if (str[i] == '|') {
      if (str[i + 1] == '|') {
        needs_quoting = true;
        break;
      }
      needs_quoting = true;
      break;
    }

    if (str[i] == ';') {
      needs_quoting = true;
      break;
    }

    if (str[i] == '&') {
      if (str[i + 1] == '&') {
        needs_quoting = true;
        break;
      }
      needs_quoting = true;
      break;
    }

    // crazy logic for enviroment variables
    if (str[i] == '$') {
      needs_quoting = true;
      break;
    }

    if (str[i] == '{') {
      needs_quoting = true;
      break;
    }

    if (str[i] == '*') {
      needs_quoting = true;
      break;
    }

    if (str[i] == '#') {
      needs_quoting = true;
    }

    // crude tilde expansion
    if (str[i] == '~') {
      needs_quoting = true;
      break;
    }
  }

  if (needs_quoting) {
    VECTOR_PUSH(*buffer, '\'');
    for (size_t i = 0; str[i] != '\0'; i++) {
      if (str[i] == '\'') {
        VECTOR_PUSH(*buffer, '\\');
        VECTOR_PUSH(*buffer, '\'');
        continue;
      }
      if (str[i] == '\\') {
        VECTOR_PUSH(*buffer, '\\');
        VECTOR_PUSH(*buffer, '\\');
        continue;
      }
      VECTOR_PUSH(*buffer, (uint8_t)str[i]);
    }
    VECTOR_PUSH(*buffer, '\'');
    return;
  }

  for (size_t i = 0; str[i] != '\0'; i++) {
    VECTOR_PUSH(*buffer, (uint8_t)str[i]);
  }
}

uint8_t *preprocess(const uint8_t *source) {
  buffer_t buffer;
  VECTOR_INIT(buffer);

  enum pp_state state = WHITESPACE;

  for (size_t i = 0; source[i] != '\0'; i++) {
    const uint8_t curr = source[i];

    switch (state) {
      case DEFAULT:
        if (isspace((int)curr)) {
          state = WHITESPACE;
          break;
        }

        if (curr == '"') {
          state = DOUBLE_QUOTE;
          break;
        }

        if (curr == '\'') {
          state = SINGLE_QUOTE;
          break;
        }

        // crazy logic for enviroment variables
        if (curr == '$') {
          i++;
          size_t env_len = 0;
          const uint8_t *env_start = source + i;
          char *env_name;

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

            env_name = malloc(env_len + 1);
            memcpy(env_name, env_start, env_len);
            env_name[env_len] = '\0';
          } else {
            for (;;) {
              if (!isalnum((int)source[i]) || source[i] == '\0') {
                i--;
                break;
              }

              i++;
              env_len++;
            }

            if (env_len == 0) {
              // push '$' onto buffer
              break;
            }

            env_name = malloc(env_len + 1);
            memcpy(env_name, env_start, env_len);
            env_name[env_len] = '\0';
          }

          char *env_value = getenv(env_name);

          if (env_name == NULL) {
            fprintf(
                stderr,
                "rash: environment variable ‘%s’ does not exist.\n",
                env_name
            );
            goto error;
          }

          insert_string(&buffer, env_value);
          continue;
        }

        if (curr == '{') {
          i++;
          const uint8_t *var_start = source + i;
          size_t var_len = 0;

          for (;;) {
            if (source[i] == '}') {
              break;
            }

            if (source[i] == '\0') {
              fprintf(stderr, "rash: expected closing ‘}’ character.\n");
              goto error;
            }

            i++;
            var_len++;
          }

          if (var_len == 0) {
            fprintf(stderr, "rash: cannot expand empty shell variable.\n");
            goto error;
          }

          char *var_name = malloc(var_len + 1);
          memcpy(var_name, var_start, var_len);
          var_name[var_len] = '\0';

          const char *var_value = var_get(var_name);
          if (var_value == NULL) {
            fprintf(
                stderr, "rash: shell variable ‘%s’ does not exist.\n", var_name
            );
            goto error;
          }

          insert_string(&buffer, var_value);
          continue;
        }

        break;
      case WHITESPACE:
        if (!isspace((int)curr)) {
          i--;
          state = DEFAULT;
          continue;
        }
        break;
      case SINGLE_QUOTE:
        if (curr == '\'') {
          state = DEFAULT;
          continue;
        }

        if (curr == '\\') {
          if (source[i + 1] == '\\') {
            VECTOR_PUSH(buffer, '\\');
            i++;
            continue;
          }
          if (source[i + 1] == '\'') {
            VECTOR_PUSH(buffer, '\'');
            i++;
            continue;
          }
        }
        break;
      case DOUBLE_QUOTE:
        if (curr == '"') {
          state = DOUBLE_QUOTE;
          continue;
        }
        if (curr == '\\') {
          if (source[i + 1] == '\\') {
            VECTOR_PUSH(buffer, '\\');
            i++;
            continue;
          }
          if (source[i + 1] == '\"') {
            VECTOR_PUSH(buffer, '\'');
            i++;
            continue;
          }
        }
        break;
      case SINGLE_LITERAL:
        state = DEFAULT;
        break;
    }

    VECTOR_PUSH(buffer, curr);
  }

  return buffer.data;

  error:
  VECTOR_DESTROY(buffer);
  return NULL;
}
