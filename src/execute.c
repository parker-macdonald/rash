#include "execute.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "./builtins/find_builtin.h"
#include "jobs.h"

static int spawn_process(char **const argv) {
  pid_t pid = fork();

  // child
  if (pid == 0) {
    int status = execvp(argv[0], argv);

    if (status == -1) {
      fprintf(stderr, "rash: ");

      if (errno != ENOENT) {
        fprintf(stderr, "execvp: ");
        perror(argv[0]);
      } else {
        fprintf(stderr, "%s: command not found\n", argv[0]);
      }
    }

    // apparently you're supposed to use _exit() inside of a child process instead of exit()
    _exit(EXIT_FAILURE);
  }
  // error forking
  else if (pid == -1) {
    perror("fork");

    return EXIT_FAILURE;
  }
  // parent process
  else {
    int status = 0;
    fg_pid = pid;

    waitpid(pid, &status, WUNTRACED);

    fg_pid = 0;

    if (recv_sigtstp) {
      recv_sigtstp = 0;

      register_stopped_job(pid);

      return 0;
    }

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
