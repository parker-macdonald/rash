// for getline
#define _POSIX_C_SOURCE 200809L

#include "enviroment.h"
#include "execute.h"
#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 256

int main(int argc, char **argv) {
  FILE *fp = NULL;
  bool interactive = false;

  if (argc == 2) {
    fp = fopen(argv[1], "r");

    if (fp == NULL) {
      perror(argv[1]);
      return 1;
    }
  } else if (argc == 1) {
    interactive = true;
    fp = stdin;
  } else {
    fprintf(stderr, "Usage: %s [FILE]", argv[1]);
  }

  char *line = NULL;
  size_t line_size = 0;

  bool should_exit = false;
  env_init();
  while (!should_exit) {
    if (interactive) {
      printf("$ ");
    }

    if (getline(&line, &line_size, fp) <= 0) {
      break;
    }

    char **tokens = get_tokens_from_line(line);

    int status = execute(tokens, &should_exit);

    free(tokens);

    // ? + = + 3 digit number (exit status is max of 255) + null terminator
    char status_env[1 + 1 + 3 + 1] = {0};

    sprintf(status_env, "?=%d", status);
    env_add(status_env);
  }
  env_destroy();

  free(line);

  fclose(fp);

  return 0;
}
