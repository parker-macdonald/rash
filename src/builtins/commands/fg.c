#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "../../jobs.h"
#include "../builtins.h"

#define BASE 10

static const char *const FG_HELP =
    "Usage: fg [JOB_ID]\n"
    "Run a paused job in the foreground based on the job id.\n"
    "If no job id is specified, the most recent job is used instead.";

int builtin_fg(char **argv) {
  int job_id;

  if (argv[1] == NULL) {
    job_id = -1;
  } else {
    if (strcmp(argv[1], "--help") == 0) {
      puts(FG_HELP);
      return EXIT_FAILURE;
    }

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
    if (job_id == -1) {
      fprintf(stderr, "fg: no running jobs\n");
    } else {
      fprintf(stderr, "fg: %d: no such job\n", job_id);
    }

    return EXIT_FAILURE;
  }

  fg_pid = pid;
  if (kill(pid, SIGCONT) != 0) {
    perror("fg: kill");
    return EXIT_FAILURE;
  }

  printf("[%d] PID: %d, continued in foreground\n", job_id, pid);

  int status;

  pid_t id = waitpid(pid, &status, WUNTRACED);
  // if this waitpid fails, something in rash has gone wrong
  assert(id != -1);

  fg_pid = 0;

  if (recv_sigtstp) {
    recv_sigtstp = 0;

    register_job(pid, JOB_STOPPED);

    return 0;
  }

  return WEXITSTATUS(status);
}
