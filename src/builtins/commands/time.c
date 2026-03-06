#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
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

  long ticks_per_sec = sysconf(_SC_CLK_TCK);
  // sysconf can only fail if name is invalid and name is always valid so...
  assert(ticks_per_sec != -1);

  struct tms tms_then;
  clock_t then = times(&tms_then);
  // times can only fail if the buffer is outside of our address space, which it
  // isn't
  assert(then != (clock_t)-1);

  execution_context ec = {
      .argv = argv + 1,
      .flags = EC_NO_WAIT | EC_DONT_REGISTER_FOREGROUND,
      .stderr_fd = -1,
      .stdin_fd = -1,
      .stdout_fd = -1
  };

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

  struct tms tms_now;
  clock_t now = times(&tms_now);
  // times can only fail if the buffer is outside of our address space, which it
  // isn't
  assert(now != (clock_t)-1);

  double elapsed = (double)(now - then) / (double)ticks_per_sec;
  double user = (double)(tms_now.tms_cutime - tms_then.tms_cutime) /
                (double)ticks_per_sec;
  double sys = (double)(tms_now.tms_cstime - tms_then.tms_cstime) /
               (double)ticks_per_sec;

  printf(
      "user time: %.3fs\n"
      "sys time: %.3fs\n"
      "elapsed: %.3fs\n",
      user,
      sys,
      elapsed
  );

  return status;
}
