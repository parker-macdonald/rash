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

static void insert_string(buf_t *buffer, const char *str) {
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
      needs_quoting = true;
      break;
    }

    // stdout redirection
    if (str[i] == '>') {
      needs_quoting = true;
      break;
    }

    // stderr redirection
    if (str[i] == '2') {
      if (str[i + 1] == '>') {
        needs_quoting = true;
        break;
      }
    }

    // pipe and logical or
    if (str[i] == '|') {
      needs_quoting = true;
      break;
    }

    // semi
    if (str[i] == ';') {
      needs_quoting = true;
      break;
    }

    // amp and logical and
    if (str[i] == '&') {
      needs_quoting = true;
      break;
    }

    // environment vars
    if (str[i] == '$') {
      needs_quoting = true;
      break;
    }

    // shell vars
    if (str[i] == '{') {
      needs_quoting = true;
      break;
    }

    // globs
    if (str[i] == '*') {
      needs_quoting = true;
      break;
    }

    // comments
    if (str[i] == '#') {
      needs_quoting = true;
    }

    // tilde expansion
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

static char *to_pattern(buf_t *buffer, size_t word_start) {
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

  VECTOR_PUSH(pattern, '\0');
  return pattern.data;
}

#define PREFORM_GLOB                                                           \
  do {                                                                         \
    if (needs_globbing) {                                                      \
      char *pattern = to_pattern(&buffer, word_start);                         \
      strings_t *matches = glob(pattern);                                      \
      if (matches == NULL) {                                                   \
        free(pattern);                                                         \
        goto error;                                                            \
      }                                                                        \
      buffer.length = word_start;                                              \
      if (matches->length == 0) {                                              \
        if (print_errors) {                                                    \
          for (size_t j = 0; pattern[j] != '\0'; j++) {                        \
            if (pattern[j] == '\033') {                                        \
              pattern[j] = '*';                                                \
            }                                                                  \
          }                                                                    \
          fprintf(                                                             \
              stderr, "rash: nothing matched glob pattern ‘%s’.\n", pattern    \
          );                                                                   \
        }                                                                      \
                                                                               \
        free(pattern);                                                         \
        VECTOR_DESTROY(*matches);                                              \
        goto error;                                                            \
      } else {                                                                 \
        for (size_t j = 0; j < matches->length; j++) {                         \
          if (j != 0) {                                                        \
            VECTOR_PUSH(buffer, ' ');                                          \
          }                                                                    \
          insert_string(&buffer, matches->data[j]);                            \
          free(matches->data[j]);                                              \
        }                                                                      \
      }                                                                        \
                                                                               \
      free(pattern);                                                           \
      VECTOR_DESTROY(*matches);                                                \
      needs_globbing = false;                                                  \
    }                                                                          \
  } while (0)

#define PUSH_STRING(buffer, str)                                               \
  do {                                                                         \
    for (size_t j = 0; str[j] != '\0'; j++) {                                  \
      VECTOR_PUSH(buffer, (uint8_t)str[j]);                                    \
    }                                                                          \
  } while (0)

static buf_t buffer;

buf_t *preprocess(const uint8_t *source, bool print_errors) {
  VECTOR_INIT(buffer);

  enum pp_state state = WHITESPACE;

  size_t word_start = 0;
  bool needs_globbing = false;

  for (size_t i = 0; source[i] != '\0'; i++) {
    const uint8_t curr = source[i];

    switch (state) {
      case DEFAULT:
        if (isspace((int)curr)) {
          PREFORM_GLOB;
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
            PUSH_STRING(buffer, "<<<");
            word_start = buffer.length;
            i += 2;
            continue;
          } else {
            PREFORM_GLOB;
            VECTOR_PUSH(buffer, '<');
            word_start = buffer.length;
            continue;
          }
        }

        // stdout redirection
        if (curr == '>') {
          if (source[i + 1] == '>') {
            PREFORM_GLOB;
            PUSH_STRING(buffer, ">>");
            word_start = buffer.length;
            i += 1;
            continue;
          }
          PREFORM_GLOB;
          VECTOR_PUSH(buffer, '>');
          word_start = buffer.length;
          continue;
        }

        if (curr == '2') {
          if (source[i + 1] == '>') {
            if (source[i + 2] == '>') {
              PREFORM_GLOB;
              PUSH_STRING(buffer, "2>>");
              word_start = buffer.length;
              i += 2;
              continue;
            }
            PREFORM_GLOB;
            PUSH_STRING(buffer, "2>");
            word_start = buffer.length;
            i += 1;
            continue;
          }
        }

        if (curr == '|') {
          if (source[i + 1] == '|') {
            PREFORM_GLOB;
            PUSH_STRING(buffer, "||");
            word_start = buffer.length;
            i += 1;
            continue;
          }
          PREFORM_GLOB;
          VECTOR_PUSH(buffer, '|');
          word_start = buffer.length;
          continue;
        }

        if (curr == ';') {
          PREFORM_GLOB;
          VECTOR_PUSH(buffer, ';');
          word_start = buffer.length;
          continue;
        }

        if (curr == '&') {
          if (source[i + 1] == '&') {
            PREFORM_GLOB;
            PUSH_STRING(buffer, "&&");
            word_start = buffer.length;
            i += 1;
            continue;
          }
          PREFORM_GLOB;
          VECTOR_PUSH(buffer, '&');
          word_start = buffer.length;
          continue;
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
                if (print_errors) {
                  fprintf(stderr, "rash: expected closing ‘}’ character.\n");
                }
                goto error;
              }

              i++;
              env_len++;
            }

            if (env_len == 0) {
              if (print_errors) {
                fprintf(
                    stderr, "rash: cannot expand empty enviroment variable.\n"
                );
              }
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
            if (print_errors) {
              fprintf(
                  stderr,
                  "rash: environment variable ‘%s’ does not exist.\n",
                  env_name
              );
            }
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
              if (print_errors) {
                fprintf(stderr, "rash: expected closing ‘}’ character.\n");
              }
              goto error;
            }

            i++;
            var_len++;
          }

          if (var_len == 0) {
            if (print_errors) {
              fprintf(stderr, "rash: cannot expand empty shell variable.\n");
            }

            goto error;
          }

          char *var_name = malloc(var_len + 1);
          memcpy(var_name, var_start, var_len);
          var_name[var_len] = '\0';

          const char *var_value = var_get(var_name);
          if (var_value == NULL) {
            if (print_errors) {
              fprintf(
                  stderr,
                  "rash: shell variable ‘%s’ does not exist.\n",
                  var_name
              );
            }

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

        if (curr == '#') {
          for (;;) {
            VECTOR_PUSH(buffer, source[i]);
            if (source[i] == '\0') {
              break;
            }
            i++;
          }
          return &buffer;
        }

        break;
      case WHITESPACE:
        if (curr == '~') {
          i++;
          const uint8_t *user_start = source + i;
          size_t user_len = 0;

          for (;;) {
            if (source[i] == '/' || source[i] == '\0' ||
                isspace((int)source[i])) {
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
              if (print_errors) {
                fprintf(stderr, "rash: cannot expand ‘~’, HOME is not set.\n");
              }
              goto error;
            }
          } else {
            char *user = malloc(user_len + 1);
            memcpy(user, user_start, user_len);
            user[user_len] = '\0';

            struct passwd *pw = getpwnam(user);
            if (pw == NULL || pw->pw_dir == NULL) {
              if (print_errors) {
                fprintf(stderr, "rash: cannot access user ‘%s’.\n", user);
              }
              free(user);
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
