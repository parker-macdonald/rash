#include "execute.h"
#include "builtins.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int spawn_process(char **const argv) {
  pid_t pid = fork();

  // child
  if (pid == 0) {
    int status = execvp(argv[0], argv);

    if (status == -1) {
      fprintf(stderr, "execvp: ");
      perror(argv[0]);
    }

    exit(EXIT_FAILURE);
  }
  // error forking
  else if (pid == -1) {
    perror("fork");

    return EXIT_FAILURE;
  }
  // parent process
  else {
    int status;

    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    return WEXITSTATUS(status);
  }
}

int execute(char **const argv) {
  if (argv[0] == NULL) {
    return EXIT_SUCCESS;
  }

  for (size_t i = 0; i < NUM_OF_BUILTINS; i++) {
    if (strcmp(argv[0], builtins[i]) == 0) {
      return (*builtin_fns[i])(argv);
    }
  }

  return spawn_process(argv);
}
