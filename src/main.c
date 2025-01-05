// for getline
#define  _POSIX_C_SOURCE 200809L

#include "execute.h"
#include "lexer.h"
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define BUFFER_SIZE 256

int main(int argc, char **argv) {
  FILE *fp = NULL;

  if (argc == 2) {
    fp = fopen(argv[1], "r");

    if (fp == NULL) {
      perror(argv[1]);
      return 1;
    }
  } else if (argc == 1) {
    fp = stdin;
  } else {
    fprintf(stderr, "Usage: %s [FILE]", argv[1]);
  }

  char* line = NULL;
  size_t line_size = 0;

  bool should_exit = false;
  while (!should_exit && getline(&line, &line_size, fp) > 0) {
    char** tokens = get_tokens_from_line(line);

    int status = execute(tokens, &should_exit);

    free(tokens);

    printf("Process ended with status: %d\n", status);
  }

  free(line);

  fclose(fp);

  return 0;
}
