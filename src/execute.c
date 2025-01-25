#include "execute.h"
#include "builtins.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int spawn_process(char **const argv, const char *const in_path,
                  const char *const out_path, const char *const err_path) {
  pid_t pid = fork();

  // child
  if (pid == 0) {
    if (out_path != NULL) {
      if (freopen(out_path, "w", stdout) == NULL) {
        perror(out_path);
        exit(EXIT_FAILURE);
      }
    }

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

  char *in_fd = NULL;
  char *out_fd = NULL;
  char *err_fd = NULL;

  for (size_t i = 0; argv[i] != NULL; i++) {
    if (strcmp(argv[i], ">") == 0) {
      if (argv[i + 1] == NULL) {
        fprintf(stderr, "Expected filename after '>'\n");
        return EXIT_FAILURE;
      }

      out_fd = argv[i + 1];

      argv[i] = NULL;
    }
  }

  return spawn_process(argv, in_fd, out_fd, err_fd);
}
