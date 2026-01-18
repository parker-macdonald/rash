#include "execute.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins/find_builtin.h"
#include "jobs.h"
#include "lib/f_error.h"
#include "lib/search_path.h"

extern char **environ;

int execute(const execution_context context) {
  if (context.argv == NULL) {
    return EXIT_SUCCESS;
  }

  builtin_t builtin = find_builtin(context.argv[0]);

  bool is_io_redirected = context.stderr_fd != -1 || context.stdin_fd != -1 ||
                          context.stdout_fd != -1;

  // no need to fork if the command is builtin and i/o isn't redirected. this
  // also is needed so that commands like export and cd change the state of
  // rash, and not the child process
  if (!is_io_redirected && builtin != NULL &&
      !(context.flags & EC_BACKGROUND_JOB) && !(context.flags & EC_NO_WAIT)) {
    return builtin(context.argv);
  }

  pid_t pid = fork();

  // child
  if (pid == 0) {
    if (tty_fd != -1 && (!(context.flags & EC_BACKGROUND_JOB) ||
                         !(context.flags & EC_DONT_REGISTER_FOREGROUND))) {
      pid_t new_pid = getpid();

      int status = setpgid(new_pid, new_pid);
      assert(status == 0);

      status = tcsetpgrp(tty_fd, new_pid);
      assert(status == 0);

      (void)signal(SIGTTOU, SIG_IGN);
      (void)signal(SIGTSTP, SIG_DFL);
    }

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

    if (builtin != NULL) {
      _exit(builtin(context.argv));
    }

    // search path for executable
    if (strchr(context.argv[0], '/') == NULL) {
      char *argv0 = search_path(context.argv[0]);

      if (argv0 == NULL) {
        f_error("%s: command not found\n", context.argv[0]);
        _exit(EXIT_FAILURE);
      }

      free(context.argv[0]);
      context.argv[0] = argv0;
    }

    int status = execve(context.argv[0], context.argv, environ);

    if (status == -1) {

      if (errno != ENOENT) {
        f_error(stderr, "rash: execve: %s\n", strerror(errno));
      } else {
        f_error(stderr, "rash: %s: command not found\n", context.argv[0]);
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
  }

  return wait_process(pid);
}

int wait_process(pid_t pid) {
  int status = 0;

  int wait_status = waitpid(pid, &status, WUNTRACED);
  reset_fg_process();

  if (wait_status == -1) {
    perror("rash: waitpid");
    return -1;
  }

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }

  if (WIFSIGNALED(status)) {
    int signal = WTERMSIG(status);

    (void)fputs(strsignal(signal), stderr);

// WCOREDUMP is from POSIX.1-2024 so it's pretty new and might not be available
// everywhere.
#ifdef WCOREDUMP
    if (WCOREDUMP(status)) {
      (void)fputs(" (core dumped)", stderr);
    }
#endif

    (void)fputc('\n', stderr);
    // exiting with a signal seems like a failure to me
    return EXIT_FAILURE;
  }

  if (WIFSTOPPED(status)) {
    putchar('\n');

    register_job(pid, JOB_STOPPED);
  }

  return 0;
}
