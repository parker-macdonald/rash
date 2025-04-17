#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "../../jobs.h"
#include "../builtins.h"

#define BASE 10

int builtin_bg(char **argv) {
  int job_id;

  if (argv[1] == NULL) {
    job_id = -1;
  } else {
    char *endptr;
    errno = 0;
    long num = strtol(argv[1], &endptr, BASE);

    if (errno != 0 || *endptr != '\0' || num < 1 || num > INT_MAX) {
      fprintf(stderr, "bg: %s: number 1 or greater expected\n", argv[1]);
      return EXIT_FAILURE;
    }

    job_id = (int)num;
  }

  job_t *job = get_job(job_id);

  if (job == NULL) {
    fprintf(stderr, "bg: %d: no such job\n", job_id);
    return EXIT_FAILURE;
  }

  kill(job->pid, SIGCONT);
  job->state = JOB_RUNNING;

  printf("[%d] PID: %d Continued in background\n", job->id, job->pid);

  return EXIT_SUCCESS;
}
