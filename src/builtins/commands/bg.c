#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins/builtins.h"
#include "jobs.h"
#include "lib/error.h"

static const char *const BG_HELP =
    "Usage: bg [JOB_ID]\n"
    "Run a paused job in the background based on the job id.\n"
    "If no job id is specified, the most recent job is used instead.";

int builtin_bg(char **argv) {
  int job_id;

  if (argv[1] == NULL) {
    job_id = -1;
  } else {
    if (strcmp(argv[1], "--help") == 0) {
      puts(BG_HELP);
      return EXIT_FAILURE;
    }

    char *endptr;
    errno = 0;
    long num = strtol(argv[1], &endptr, 10);

    if (errno != 0 || *endptr != '\0' || num < 1 || num > INT_MAX) {
      error_f("bg: %s: number 1 or greater expected\n", argv[1]);
      return EXIT_FAILURE;
    }

    job_id = (int)num;
  }

  job_t *job = get_job(job_id);

  if (job == NULL) {
    if (job_id == -1) {
      error_f("bg: no running jobs\n");
    } else {
      error_f("bg: %d: no such job\n", job_id);
    }

    return EXIT_FAILURE;
  }

  if (job->state == JOB_RUNNING) {
    error_f("bg: %d: job already running\n", job->id);
    return EXIT_FAILURE;
  }

  if (kill(job->pid, SIGCONT) != 0) {
    perror("bg: kill");
    return EXIT_FAILURE;
  }

  job->state = JOB_RUNNING;

  printf("[%d] PID: %d, continued in background\n", job->id, job->pid);

  return EXIT_SUCCESS;
}
