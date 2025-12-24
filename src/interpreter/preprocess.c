#include "preprocess.h"

#include <ctype.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "../shell_vars.h"
#include "../vec_types.h"
#include "../vector.h"
#include "glob.h"

enum pp_state {
  DEFAULT,
  WHITESPACE,
  SINGLE_QUOTE,
  DOUBLE_QUOTE,
  SINGLE_LITERAL
};

void insert_string(buf_t *buffer, const char *str) {
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

char *to_pattern(buf_t *buffer, size_t word_start) {
  VECTOR(char) pattern;
  VECTOR_INIT(pattern);

  enum pp_state state = DEFAULT;

  for (size_t i = word_start; i < buffer->length; i++) {
    switch (state) {
      case DEFAULT:
        if (buffer->data[i] == '"') {
          state = DOUBLE_QUOTE;
          break;
        }

        if (buffer->data[i] == '\'') {
          state = SINGLE_QUOTE;
          break;
        }

        if (buffer->data[i] == '\\') {
          state = SINGLE_LITERAL;
          break;
        }

        // stdin redirection
        if (buffer->data[i] == '<') {
          if (buffer->data[i + 1] == '<' && buffer->data[i + 2] == '<') {
            goto success;
          } else {
            goto success;
          }
        }

        // stdout redirection
        if (buffer->data[i] == '>') {
          if (buffer->data[i + 1] == '>') {
            goto success;
          }
          goto success;
        }

        if (buffer->data[i] == '2') {
          if (buffer->data[i + 1] == '>') {
            if (buffer->data[i + 2] == '>') {
              goto success;
            }
            goto success;
          }
        }

        if (buffer->data[i] == '|') {
          if (buffer->data[i + 1] == '|') {
            goto success;
          }
          goto success;
        }

        if (buffer->data[i] == ';') {
          goto success;
        }

        if (buffer->data[i] == '&') {
          if (buffer->data[i + 1] == '&') {
            goto success;
          }
          goto success;
        }

        if (buffer->data[i] == '#') {
          goto success;
        }

        VECTOR_PUSH(pattern, (char)buffer->data[i]);
        break;

      case DOUBLE_QUOTE:
        if (buffer->data[i] == '"') {
          state = DEFAULT;
          break;
        }

        if (buffer->data[i] == '\\') {
          if (buffer->data[i + 1] == '\\') {
            VECTOR_PUSH(pattern, '\\');
            i++;
            break;
          }
          if (buffer->data[i + 1] == '\"') {
            VECTOR_PUSH(pattern, '\"');
            i++;
            break;
          }
        }

        VECTOR_PUSH(pattern, (char)buffer->data[i]);
        break;

      case SINGLE_QUOTE:
        if (buffer->data[i] == '\'') {
          state = DEFAULT;
          break;
        }

        if (buffer->data[i] == '\\') {
          if (buffer->data[i + 1] == '\\') {
            VECTOR_PUSH(pattern, '\\');
            i++;
            break;
          }
          if (buffer->data[i + 1] == '\'') {
            VECTOR_PUSH(pattern, '\'');
            i++;
            break;
          }
        }

        VECTOR_PUSH(pattern, (char)buffer->data[i]);
        break;

      case SINGLE_LITERAL:
        VECTOR_PUSH(pattern, (char)buffer->data[i]);
        state = DEFAULT;
        break;

      // this will never happen, just silencing a compiler error
      case WHITESPACE:
        break;
    }
  }

success:
  VECTOR_PUSH(pattern, '\0');
  return pattern.data;
}

#define PREFORM_GLOB                                                           \
  if (needs_globbing) {                                                        \
    char *pattern = to_pattern(&buffer, word_start);                           \
    strings_t *matches = glob(pattern);                                        \
    free(pattern);                                                             \
                                                                               \
    buffer.length = word_start == 0 ? 0 : word_start - 1;                      \
    for (size_t j = 0; j < matches->length; j++) {                             \
      VECTOR_PUSH(buffer, ' ');                                                \
      insert_string(&buffer, matches->data[j]);                                \
      free(matches->data[j]);                                                  \
    }                                                                          \
                                                                               \
    VECTOR_DESTROY(*matches);                                                  \
    needs_globbing = false;                                                    \
  }

#define PUSH_STRING(buffer, str)                                               \
  do {                                                                         \
    for (size_t j = 0; str[j] != '\0'; j++) {                                  \
      VECTOR_PUSH(buffer, str[i]);                                             \
    }                                                                          \
  } while (0)

static buf_t buffer;

buf_t *preprocess(const uint8_t *source) {
  VECTOR_INIT(buffer);

  enum pp_state state = WHITESPACE;

  size_t word_start = 0;
  bool needs_globbing = false;

  for (size_t i = 0; source[i] != '\0'; i++) {
    const uint8_t curr = source[i];

    switch (state) {
      case DEFAULT:
        if (isspace((int)curr)) {
          PREFORM_GLOB
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

        // stdin redirection
        if (curr == '<') {
          if (source[i + 1] == '<' && source[i + 2] == '<') {
            PREFORM_GLOB;
            word_start = buffer.length + 4;
            break;
          } else {
            PREFORM_GLOB;
            word_start = buffer.length + 3;
            break;
          }
        }

        // stdout redirection
        if (curr == '>') {
          if (source[i + 1] == '>') {
            PREFORM_GLOB;
            word_start = buffer.length + 3;
            break;
          }
          PREFORM_GLOB;
          word_start = buffer.length + 2;
          break;
        }

        if (curr == '2') {
          if (source[i + 1] == '>') {
            if (source[i + 2] == '>') {
              PREFORM_GLOB;
              word_start = buffer.length + 4;
              break;
            }
            PREFORM_GLOB;
            word_start = buffer.length + 3;
            break;
          }
        }

        if (curr == '|') {
          if (source[i + 1] == '|') {
            PREFORM_GLOB;
            word_start = buffer.length + 3;
            break;
          }
          PREFORM_GLOB;
          word_start = buffer.length + 2;
          break;
        }

        if (curr == ';') {
          PREFORM_GLOB;
          word_start = buffer.length + 2;
          break;
        }

        if (curr == '&') {
          if (source[i + 1] == '&') {
            PREFORM_GLOB;
            word_start = buffer.length + 3;
            break;
          }
          PREFORM_GLOB;
          word_start = buffer.length + 2;
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
            free(env_name);
            goto error;
          }

          free(env_name);
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
            free(var_name);
            goto error;
          }

          free(var_name);
          insert_string(&buffer, var_value);
          continue;
        }

        if (curr == '*') {
          needs_globbing = true;
          VECTOR_PUSH(buffer, '\033');
          continue;
        }

        break;
      case WHITESPACE:
        if (curr == '~') {
          i++;
          const uint8_t *user_start = source + i;
          size_t user_len = 0;

          for (;;) {
            if (source[i] == '/' || source[i] == '\0') {
              break;
            }
            user_len++;
            i++;
          }

          if (user_len == 0) {
            char *home = getenv("HOME");
            if (home != NULL) {
              insert_string(&buffer, home);
            } else {
              fprintf(stderr, "rash: cannot expand ‘~’, HOME is not set.\n");
              goto error;
            }
          } else {
            char *user = malloc(user_len + 1);
            memcpy(user, user_start, user_len);
            user[user_len] = '\0';

            struct passwd *pw = getpwnam(user);
            if (pw == NULL || pw->pw_dir == NULL) {
              fprintf(stderr, "rash: cannot access user ‘%s’.\n", user);
              goto error;
            }

            insert_string(&buffer, pw->pw_dir);
          }
        }

        if (!isspace((int)curr)) {
          word_start = buffer.length;
          i--;
          state = DEFAULT;
          continue;
        }
        break;
      case SINGLE_QUOTE:
        if (curr == '\'') {
          state = DEFAULT;
          break;
        }

        if (curr == '\\' && source[i + 1] == '\'') {
          VECTOR_PUSH(buffer, '\\');
          i++;
        }
        break;
      case DOUBLE_QUOTE:
        if (curr == '"') {
          state = DOUBLE_QUOTE;
          break;
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

    VECTOR_PUSH(buffer, source[i]);
  }

  PREFORM_GLOB;

  VECTOR_PUSH(buffer, '\0');

  return &buffer;

error:
  VECTOR_DESTROY(buffer);
  return NULL;
}
