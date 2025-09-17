#include "lexer.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "execute.h"
#include "vector.h"

enum lexer_state {
  DEFAULT,
  WHITESPACE,
  SINGLE_QUOTE,
  DOUBLE_QUOTE,
  VAR_EXPANSION,
  SINGLE_LITERAL,
  OUTPUT_REDIRECT,
  OUTPUT_REDIRECT_APPEND,
  INPUT_REDIRECT
};

optional_exec_context get_tokens_from_line(const uint8_t *const line) {
  VECTOR(uint8_t) buffer;
  VECTOR_INIT(buffer);

  VECTOR(uintptr_t) tokens;
  VECTOR_INIT(tokens);

  VECTOR(char) stdout_path;
  VECTOR_INIT(stdout_path);

  VECTOR(char) stdin_path;
  VECTOR_INIT(stdin_path);

  size_t env_start = 0;
  size_t token_start = 0;

  bool flag_to_fix_stupid_edge_case = false;

  enum lexer_state state = WHITESPACE;
  enum lexer_state prev_state = WHITESPACE;
  enum lexer_state next_state = DEFAULT;

  execution_context context = {
      .flags = 0, .stderr_fd = -1, .stdin_fd = -1, .stdout_fd = -1};

  size_t i;
  for (i = 0; line[i] != '\0'; i++) {
    const uint8_t curr = line[i];

    switch (state) {
      case DEFAULT:
        if (curr == '>') {
          prev_state = state;
          state = WHITESPACE;

          if (line[i + 1] == '>') {
            i++;
            next_state = OUTPUT_REDIRECT_APPEND;
          } else {
            next_state = OUTPUT_REDIRECT;
          }

          break;
        }

        if (curr == '<') {
          prev_state = state;
          state = WHITESPACE;
          next_state = INPUT_REDIRECT;

          break;
        }

        if (flag_to_fix_stupid_edge_case) {
          flag_to_fix_stupid_edge_case = false;
          token_start = i;
          VECTOR_PUSH(tokens, buffer.length);
        }

        if (isspace((int)curr)) {
          prev_state = state;
          state = WHITESPACE;
          next_state = DEFAULT;

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
          state = next_state;
          flag_to_fix_stupid_edge_case = state == DEFAULT;
          i--;
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
        if ((i == env_start + 1 && isdigit((int)curr)) || curr == '?') {
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
      case OUTPUT_REDIRECT:
        if (isspace((int)curr)) {
          VECTOR_PUSH(stdout_path, '\0');
          int fd = open(stdout_path.data, O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (fd == -1) {
            perror(stdout_path.data);
            goto error;
          }
          context.stdout_fd = fd;
          break;
        }

        VECTOR_PUSH(stdout_path, (char)curr);
        break;
      case OUTPUT_REDIRECT_APPEND:
        if (isspace((int)curr)) {
          VECTOR_PUSH(stdout_path, '\0');
          int fd = open(stdout_path.data, O_WRONLY | O_CREAT | O_APPEND, 0644);
          if (fd == -1) {
            perror(stdout_path.data);
            goto error;
          }
          context.stdout_fd = fd;
          break;
        }

        VECTOR_PUSH(stdout_path, (char)curr);
        break;
      case INPUT_REDIRECT:
        if (isspace((int)curr)) {
          VECTOR_PUSH(stdin_path, '\0');
          int fd = open(stdin_path.data, O_RDONLY);
          if (fd == -1) {
            perror(stdin_path.data);
            goto error;
          }
          context.stdin_fd = fd;
          break;
        }

        VECTOR_PUSH(stdin_path, (char)curr);
        break;
    }
  }

  switch (state) {
    case DEFAULT:
    case WHITESPACE:
      switch (next_state) {
        case OUTPUT_REDIRECT:
          fprintf(stderr, "Expected file name after ‘>’.\n");
          goto error;
        case OUTPUT_REDIRECT_APPEND:
          fprintf(stderr, "Expected file name after ‘>>’.\n");
          goto error;
        case INPUT_REDIRECT:
          fprintf(stderr, "Expected file name after ‘<’.\n");
          goto error;
        default:
          break;
      }
      break;
    case VAR_EXPANSION: {
      const size_t env_len = i - env_start - 1;
      char *env_name = malloc(env_len + 1);
      strncpy(env_name, (const char *)&line[env_start + 1], env_len);
      env_name[env_len] = '\0';

      const uint8_t *env_value = (uint8_t *)getenv(env_name);

      free(env_name);

      if (env_value != NULL) {
        for (size_t j = 0; env_value[j] != '\0'; j++) {
          VECTOR_PUSH(buffer, env_value[j]);
        }
      }

      break;
    }
    case SINGLE_LITERAL:
      fprintf(stderr, "Expected character after ‘\\’.\n");
      goto error;
    case SINGLE_QUOTE:
      fprintf(stderr, "Expected closing ‘'’ character.\n");
      goto error;
    case DOUBLE_QUOTE:
      fprintf(stderr, "Expected closing ‘\"’ character.\n");
      goto error;
    case OUTPUT_REDIRECT: {
      VECTOR_PUSH(stdout_path, '\0');
      int fd = open(stdout_path.data, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        perror(stdout_path.data);
        goto error;
      }
      context.stdout_fd = fd;
      break;
    }
    case OUTPUT_REDIRECT_APPEND: {
      VECTOR_PUSH(stdout_path, '\0');
      int fd = open(stdout_path.data, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        perror(stdout_path.data);
        goto error;
      }
      context.stdout_fd = fd;
      break;
    }
    case INPUT_REDIRECT: {
      VECTOR_PUSH(stdin_path, '\0');
      int fd = open(stdin_path.data, O_RDONLY);
      if (fd == -1) {
        perror(stdin_path.data);
        goto error;
      }
      context.stdin_fd = fd;
      break;
    }
  }

  VECTOR_DESTROY(stdout_path);
  VECTOR_DESTROY(stdin_path);

  VECTOR_PUSH(buffer, '\0');

  for (size_t j = 0; j < tokens.length; j++) {
    tokens.data[j] += (size_t)buffer.data;
  }

  VECTOR_PUSH(tokens, 0x0);

  context.argv = (char **)tokens.data;
  return (optional_exec_context){.has_value = true, .value = context};

error:
  VECTOR_DESTROY(tokens);
  VECTOR_DESTROY(buffer);
  VECTOR_DESTROY(stdout_path);
  VECTOR_DESTROY(stdin_path);

  return (optional_exec_context){.has_value = false};
}
