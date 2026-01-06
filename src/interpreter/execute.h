#ifndef EXECUTE_H
#define EXECUTE_H

#include <sys/types.h>

#include "../optional.h"

// this flag tells execute not to wait for the program to finish and add it to
// the registered jobs
#define EC_BACKGROUND_JOB (1)
// this flag tells execute not to wait for the program to finish and instead
// return the pid (pids and ints are the same size). this flag is mutually
// exclusive with EC_BACKGROUND_JOB
#define EC_NO_WAIT (1 << 1)
// this flag tells execute to not call tcsetpgrp (sets the process to the
// foreground process of the tty) on the spawned process. this is useful for
// spawning pipelines where only the last command of the pipeline is the only
// foreground process. this flag is also implied by EC_BACKGROUND_JOB
#define EC_DONT_REGISTER_FOREGROUND (1 << 2)

typedef struct {
  // arguments for command
  char **argv;
  // fd to redirect stdout, or -1
  int stdout_fd;
  // fd to redirect stdin, or -1
  int stdin_fd;
  // fd to redirect stderr, or -1
  int stderr_fd;
  int flags;
} execution_context;

int execute(const execution_context context);

/**
 * @brief this function is called by execute to handle errors and wait on
 * running processes, if you run a process in the foreground, pretty please call
 * this function (looking at you fg command).
 * @param pid the process to do things with
 * @return the exit status of the program or -1 if waitpid encounters an error.
 */
int wait_process(pid_t pid);

#endif
