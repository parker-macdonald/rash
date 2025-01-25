#include "vector.h"
#include <ctype.h>
#include <stdio.h>

char **get_tokens_from_line(char *const line) {
  VECTOR(char *) tokens;
  VECTOR_INIT(tokens);

  char *token;

  for (size_t i = 0; line[i] != '\0'; i++) {
    if (isspace(line[i])) {
      continue;
    }
    token = line + i;

    if (line[i] == '"') {
      i++;
      token++;

      for (;; i++) {
        if (line[i] == '\0') {
          fprintf(stderr, "Closing quote expected.\n");
          VECTOR_DESTROY(tokens);
          return NULL;
        }

        if (line[i] == '"') {
          line[i] = '\0';
          break;
        }
      }

      VECTOR_PUSH(tokens, token);
      continue;
    }

    for (; !isspace(line[i]); i++)
      ;

    line[i] = '\0';

    if (token[0] == '$') {
      if (token[1] != '\0') {
        char *env_var = getenv(token + 1);

        if (env_var != NULL)
          VECTOR_PUSH(tokens, env_var);
        continue;
      }
    }

    VECTOR_PUSH(tokens, token);
  }

  VECTOR_PUSH(tokens, NULL);

  return tokens.data;
}
