#include "execute.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../builtins/find_builtin.h"
#include "../jobs.h"
#include "../search_path.h"

extern char **environ;

int execute(const execution_context context) {
  if (context.argv == NULL) {
    return EXIT_SUCCESS;
  }

  builtin_t builtin = find_builtin(context.argv[0]);

  if (builtin != NULL) {
    return builtin(context.argv);
  }

  pid_t pid = fork();

  // child
  if (pid == 0) {
    if (context.stdout_fd != -1) {
      close(STDOUT_FILENO);

      int new_fd = dup(context.stdout_fd);

      assert(new_fd == STDOUT_FILENO);
    }

    if (context.stderr_fd != -1) {
      close(STDERR_FILENO);

      int new_fd = dup(context.stderr_fd);

      assert(new_fd == STDERR_FILENO);
    }

    if (context.stdin_fd != -1) {
      close(STDIN_FILENO);

      int new_fd = dup(context.stdin_fd);

      assert(new_fd == STDIN_FILENO);
    }

    // search path for executable
    if (strchr(context.argv[0], '/') == NULL) {
      char *argv0 = search_path(context.argv[0]);

      if (argv0 == NULL) {
        fprintf(stderr, "%s: command not found\n", context.argv[0]);
        _exit(EXIT_FAILURE);
      }

      free(context.argv[0]);
      context.argv[0] = argv0;
    }

    int status = execve(context.argv[0], context.argv, environ);

    if (status == -1) {
      fprintf(stderr, "rash: ");

      if (errno != ENOENT) {
        fprintf(stderr, "execvp: ");
        perror(context.argv[0]);
      } else {
        fprintf(stderr, "%s: command not found\n", context.argv[0]);
      }
    }

    // apparently you're supposed to use _exit() inside of a child process
    // instead of exit()
    _exit(EXIT_FAILURE);
  }
  // error forking
  else if (pid == -1) {
    perror("fork");

    if (context.stderr_fd != -1) {
      close(context.stderr_fd);
    }
    if (context.stdin_fd != -1) {
      close(context.stdin_fd);
    }
    if (context.stdout_fd != -1) {
      close(context.stdout_fd);
    }

    if (context.flags & EC_NO_WAIT) {
      return -1;
    }

    return EXIT_FAILURE;
  }
  // parent process
  else {
    if (context.stderr_fd != -1) {
      close(context.stderr_fd);
    }
    if (context.stdin_fd != -1) {
      close(context.stdin_fd);
    }
    if (context.stdout_fd != -1) {
      close(context.stdout_fd);
    }

    if (context.flags & EC_NO_WAIT) {
      return pid;
    }

    if (context.flags & EC_BACKGROUND_JOB) {
      register_job(pid, JOB_RUNNING);

      return EXIT_SUCCESS;
    }

    int status = 0;
    fg_pid = pid;

    pid_t id = waitpid(pid, &status, WUNTRACED);
    assert(id != -1);

    fg_pid = 0;

    if (recv_sigtstp) {
      recv_sigtstp = 0;

      register_job(pid, JOB_STOPPED);

      return 0;
    }

    return WEXITSTATUS(status);
  }

  // will never be reached
  return EXIT_SUCCESS;
}
