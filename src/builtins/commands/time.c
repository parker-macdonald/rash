#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "interpreter/execute.h"
#include "lib/error.h"

static const char *const TIME_HELP =
    "Usage: time COMMAND [ARGS...]\n"
    "Run the specified command with the specified args and see how long it\n"
    "takes to execute.";

int builtin_time(char **argv) {
  if (argv[1] == NULL) {
    puts(TIME_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    puts(TIME_HELP);
    return EXIT_SUCCESS;
  }

  ExecutionContext ec = {
      .argv = argv + 1,
      .flags = EC_NO_WAIT,
      .stderr_fd = -1,
      .stdin_fd = -1,
      .stdout_fd = -1
  };

  struct rusage usage_then;
  if (getrusage(RUSAGE_CHILDREN, &usage_then) == -1) {
    perror("time: getrusage");
    return EXIT_FAILURE;
  }

  struct timespec then;
  if (clock_gettime(CLOCK_MONOTONIC, &then) == -1) {
    perror("time: clock_gettime");
    return EXIT_FAILURE;
  }

  pid_t pid = execute(ec);

  if (pid == -1) {
    error("time: failed to execute command.\n");
    return EXIT_FAILURE;
  }

  int status = wait_process(pid);

  if (status == -1) {
    // wait_process will print an error message
    return EXIT_FAILURE;
  }

  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
    perror("time: clock_gettime");
    return EXIT_FAILURE;
  }

  struct rusage usage_now;
  if (getrusage(RUSAGE_CHILDREN, &usage_now) == -1) {
    perror("time: getrusage");
    return EXIT_FAILURE;
  }

  double elapsed = ((double)now.tv_sec + (double)now.tv_nsec / 1e9) -
                   ((double)then.tv_sec + (double)then.tv_nsec / 1e9);
  double user = ((double)usage_now.ru_utime.tv_sec +
                 (double)usage_now.ru_utime.tv_usec / 1e6) -
                ((double)usage_then.ru_utime.tv_sec +
                 (double)usage_then.ru_utime.tv_usec / 1e6);
  double sys = ((double)usage_now.ru_stime.tv_sec +
                (double)usage_now.ru_stime.tv_usec / 1e6) -
               ((double)usage_then.ru_stime.tv_sec +
                (double)usage_then.ru_stime.tv_usec / 1e6);

  (void)fprintf(
      stderr,
      "\nuser time: %.3fs\n"
      "sys time:  %.3fs\n"
      "elapsed:   %.3fs\n",
      user,
      sys,
      elapsed
  );

  return status;
}
