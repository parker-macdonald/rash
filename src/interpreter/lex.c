#include "lex.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../vector.h"

const char *const TOKEN_NAMES[] = {"STRING",
                                   "STDIN_REDIR",
                                   "STDIN_REDIR_STRING",
                                   "STDOUT_REDIR",
                                   "STDOUT_REDIR_APPEND",
                                   "STDERR_REDIR",
                                   "STDERR_REDIR_APPEND",
                                   "PIPE",
                                   "WILDCARD",
                                   "QUESTION_MARK",
                                   "BRACKET_OPEN",
                                   "BRACKET_CLOSE",
                                   "BRACE_OPEN",
                                   "BRACE_CLOSE",
                                   "DOUBLE_DOT",
                                   "SEMI",
                                   "LOGICAL_AND",
                                   "LOGICAL_OR",
                                   "END"};

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

token_t *lex(const uint8_t *const source) {
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
            VECTOR_PUSH(tokens, ((token_t){.type = STRING, .data = buffer.data}));
            VECTOR_INIT(buffer);
          }

          break;
        }

        // stdin redirection
        if (curr == '<') {
          if (source[i + 1] == '<') {
            if (source[i + 2] == '<') {
              // the token <<<
              ADD_NONSTR_TOKEN(STDIN_REDIR_STRING);
              i += 2;
              continue;
            }
          } else {
            // the token <<
            ADD_NONSTR_TOKEN(STDIN_REDIR);
            i++;
            continue;
          }
        }

        // stdout redirection
        if (curr == '>') {
          if (source[i + 1] == '>') {
            // the token >>
            ADD_NONSTR_TOKEN(STDOUT_REDIR_APPEND);
            i++;
            continue;
          } else {
            // the token >
            ADD_NONSTR_TOKEN(STDOUT_REDIR);
            continue;
          }
        }

        if (curr == '2') {
          if (source[i + 1] == '>') {
            if (source[i + 2] == '>') {
              // the token 2>>
              ADD_NONSTR_TOKEN(STDERR_REDIR_APPEND);
              i += 2;
              continue;
            } else {
              ADD_NONSTR_TOKEN(STDERR_REDIR);
              i++;
              continue;
            }
          }
        }

        if (curr == '|') {
          ADD_NONSTR_TOKEN(PIPE);
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

  ADD_NONSTR_TOKEN(END);

  return tokens.data;

error:
  VECTOR_DESTROY(buffer);
  VECTOR_DESTROY(tokens);

  return NULL;
}
