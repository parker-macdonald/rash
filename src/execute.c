#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "builtins.h"
#include "enviroment.h"
#include "execute.h"

int spawn_process(char **const argv) {
  pid_t pid = fork();

  // child
  if (pid == 0) {
    char** envp = env_get_array();
    int status = execvpe(argv[0], argv, envp);
    free(envp);

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

int execute(char **const argv, bool *should_exit) {
  if (argv[0] == NULL) {
    return EXIT_SUCCESS;
  }
  
  for (size_t i = 0; i < NUM_OF_BUILTINS; i++) {
    if (strcmp(argv[0], builtins[i]) == 0) {
      if (i == EXIT) {
        *should_exit = true;
        return EXIT_SUCCESS;
      }
      return (*builtin_fns[i])(argv);
    }
  }

  return spawn_process(argv);
}