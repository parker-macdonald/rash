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

  struct tms unused;
  clock_t then = times(&unused);
  // times can only fail if the buffer is outside of our address space, which it
  // isn't
  assert(then != -1);

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
  assert(now != -1);

  long elapsed_sec = (now - then) / ticks_per_sec;
  long elapsed_subsec = (now - then) % ticks_per_sec * 1000 / ticks_per_sec;

  long user_time_sec = tms_now.tms_cutime / ticks_per_sec;
  long user_time_subsec =
      tms_now.tms_cutime % ticks_per_sec * 1000 / ticks_per_sec;

  long sys_time_sec = tms_now.tms_cstime / ticks_per_sec;
  long sys_time_subsec =
      tms_now.tms_cstime % ticks_per_sec * 1000 / ticks_per_sec;

  printf(
      "user time: %ld.%03lds\n"
      "sys time: %ld.%03lds\n"
      "elapsed: %ld.%03lds\n",
      user_time_sec,
      user_time_subsec,
      sys_time_sec,
      sys_time_subsec,
      elapsed_sec,
      elapsed_subsec
  );

  return status;
}
