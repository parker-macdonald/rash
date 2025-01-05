#include "vector.h"
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

  char buffer[BUFFER_SIZE];

  while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
    char* pathname = strtok(buffer, " \t\n");
    VECTOR(char*) args;
    VECTOR_INIT(args);

    VECTOR_PUSH(args, pathname);

    char* token;

    while ((token = strtok(NULL, " \t\n")) != NULL) {
      VECTOR_PUSH(args, token);
    }

    VECTOR_PUSH(args, NULL);

    pid_t pid = fork();

    if (pid == 0) {
      int status = execvp(pathname, args.data);

      if (status == -1) {
        perror(pathname);
      }
    }

    if (pid == -1) {
      perror("fork");
    }

    VECTOR_DESTROY(args);
  }

  fclose(fp);

  return 0;
}
