#include "execute.h"
#include "./builtins/find_builtin.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern volatile sig_atomic_t spawned_pid;

int spawn_process(char **const argv) {
  pid_t pid = fork();

  // child
  if (pid == 0) {
    int status = execvp(argv[0], argv);

    if (status == -1) {
      fprintf(stderr, "rash: ");

      if (errno != ENOENT) {
        fprintf(stderr, "execvp: ");
      }

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
    spawned_pid = pid;

    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    spawned_pid = 0;
    return WEXITSTATUS(status);
  }

  // will never be reached
  return EXIT_SUCCESS;
}

int execute(char **const argv) {
  if (argv[0] == NULL) {
    return EXIT_SUCCESS;
  }

  builtin_t builtin = find_builtin(argv[0]);

  if (builtin != NULL) {
    return builtin(argv);
  }

  return spawn_process(argv);
}
