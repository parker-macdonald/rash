#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "../../jobs.h"
#include "../builtins.h"

#define BASE 10

int builtin_fg(char **argv) {
  int job_id;

  if (argv[1] == NULL) {
    job_id = -1;
  } else {
    char *endptr;
    errno = 0;
    long num = strtol(argv[1], &endptr, BASE);

    if (errno != 0 || *endptr != '\0' || num < 1 || num > INT_MAX) {
      fprintf(stderr, "fg: %s: number 1 or greater expected\n", argv[1]);
      return EXIT_FAILURE;
    }

    job_id = (int)num;
  }

  pid_t pid = get_pid_and_remove(&job_id);

  if (pid == 0) {
    fprintf(stderr, "fg: %d: no such job\n", job_id);
    return EXIT_FAILURE;
  }

  fg_pid = pid;
  kill(pid, SIGCONT);

  printf("[%d] PID: %d Continued in foreground\n", job_id, pid);

  int status;

  waitpid(pid, &status, WUNTRACED);

  fg_pid = 0;

  if (recv_sigtstp) {
    recv_sigtstp = 0;

    register_job(pid, JOB_STOPPED);

    return 0;
  }

  return WEXITSTATUS(status);
}
