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

#include "builtins/builtins.h"
#include "jobs.h"
#include "lib/error.h"
#include "lib/search_path.h"
#include "global.h"
#include "lib/sys.h"

extern char **environ;

int execute(ExecutionContext context) {
  if (context.argv == NULL) {
    return EXIT_SUCCESS;
  }

  BuiltinFunc builtin = find_builtin(&instance.builtins, context.argv[0]);

  bool is_io_redirected = context.stderr_fd != -1 || context.stdin_fd != -1 ||
                          context.stdout_fd != -1;

  // no need to fork if the command is builtin and i/o isn't redirected. this
  // also is needed so that commands like export and cd change the state of
  // rash, and not the child process
  if (
    !is_io_redirected && builtin != NULL &&
    !(context.flags & EC_BACKGROUND_JOB) &&
    !(context.flags & EC_NO_WAIT)
  ) {
    return builtin(context.argv);
  }

  pid_t pid = fork();

  // child
  if (pid == 0) {
    if (
      instance.tty_fd != -1 &&
      !(
        (context.flags & EC_BACKGROUND_JOB) ||
        (context.flags & EC_DONT_REGISTER_FOREGROUND)
      )
    ) {
      pid_t new_pid = getpid();

      rash_assert(setpgid(new_pid, new_pid) == 0, "setpgid failed");

      rash_assert(tcsetpgrp(instance.tty_fd, new_pid) == 0, "tcsetpgrp failed");
    }

    signal_assert(SIGTTOU, SIG_DFL);
    signal_assert(SIGTSTP, SIG_DFL);
    signal_assert(SIGINT, SIG_DFL);

    if (context.stdout_fd != -1) {
      dup2_assert(context.stdout_fd, STDOUT_FILENO);
      close_assert(context.stdout_fd);
    }

    if (context.stderr_fd != -1) {
      dup2_assert(context.stderr_fd, STDERR_FILENO);
      close_assert(context.stderr_fd);
    }

    if (context.stdin_fd != -1) {
      dup2_assert(context.stdin_fd, STDIN_FILENO);
      close_assert(context.stdin_fd);
    }

    if (builtin != NULL) {
      // using _exit instead of exit so we don't trigger the atexit function
      // which kills all child processes.
      _exit(builtin(context.argv));
    }

    const char *exec_path = context.argv[0];

    // search path for executable
    if (strchr(context.argv[0], '/') == NULL) {
      // this is technically a memory leak since search_path returns a malloc'd
      // string, but we exit unconditionally after this so it doesn't really
      // matter
      exec_path = search_path(context.argv[0]);

      if (exec_path == NULL) {
        error_f("%s: command not found\n", context.argv[0]);
        // using _exit instead of exit so we don't trigger the atexit function
        // which kills all child processes.
        _exit(EXIT_FAILURE);
      }
    }

    execve(exec_path, context.argv, environ);

    // execve failed if we get here
    if (errno != ENOENT) {
      error_f("rash: execve: %s\n", strerror(errno));
    } else {
      error_f("rash: %s: command not found\n", context.argv[0]);
    }

    // using _exit instead of exit so we don't trigger the atexit function
    // which kills all child processes.
    _exit(EXIT_FAILURE);
  }
  // in the parent now

  if (context.stderr_fd != -1) {
    close_assert(context.stderr_fd);
  }
  if (context.stdin_fd != -1) {
    close_assert(context.stdin_fd);
  }
  if (context.stdout_fd != -1) {
    close_assert(context.stdout_fd);
  }

  // error forking
  if (pid == -1) {
    perror("fork");

    // we should return a -1 to show there's no pid
    if (context.flags & EC_NO_WAIT) {
      return -1;
    }

    return EXIT_FAILURE;
  }

  if (context.flags & EC_NO_WAIT) {
    return pid;
  }

  if (context.flags & EC_BACKGROUND_JOB) {
    jobs_register(&instance.jobs, pid, JOB_RUNNING);

    return EXIT_SUCCESS;
  }

  return wait_process(pid);
}

int wait_process(pid_t pid) {
  int status = 0;

  int wait_status = waitpid(pid, &status, WUNTRACED);
  jobs_set_rash_to_foreground(&instance.jobs);

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

    jobs_register(&instance.jobs, pid, JOB_STOPPED);
  }

  return 0;
}

void execution_context_destroy(ExecutionContext *ec) {
	if (ec->stderr_fd != -1) {
		close_assert(ec->stderr_fd);
	}
	if (ec->stdin_fd != -1) {
		close_assert(ec->stdin_fd);
	}
	if (ec->stdout_fd != -1) {
		close_assert(ec->stdout_fd);
	}
}