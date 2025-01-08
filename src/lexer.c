#include "enviroment.h"
#include "vector.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static VECTOR(char *) tokens;

char **get_tokens_from_line(char *const line) {
  VECTOR_INIT(tokens);

  char *pathname = strtok(line, " \t\r\n");

  VECTOR_PUSH(tokens, pathname);

  char *token;

  while ((token = strtok(NULL, " \t\n")) != NULL) {
    if (token[0] == '$') {
      if (token[1] != '\0') {
        char* env_var = env_get(token + 1);

        VECTOR_PUSH(tokens, env_var);
        continue;
      }
    }
    VECTOR_PUSH(tokens, token);
  }

  VECTOR_PUSH(tokens, NULL);

  return tokens.data;
}