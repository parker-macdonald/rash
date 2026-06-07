#include "jobs.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "interactive.h"
#include "lib/error.h"
#include "lib/sys.h"

#define JOB_TABLE_SIZE 1024

Job job_table[JOB_TABLE_SIZE];

const char *const JOB_STATUSES[NUM_JOB_STATUSES] = {"Exited", "Stopped",
                                                    "Running"};

static pid_t root_pid;
int tty_fd = -1;

static sigset_t oldset;

static void kill_all_children(void) {

  for (size_t i = 0; i < JOB_TABLE_SIZE; i++) {
    Job *current = job_table + i;

    switch (current->state) {
    case JOB_STOPPED:
      rash_kill(current->pid, SIGCONT);
    case JOB_RUNNING:
      rash_kill(current->pid, SIGHUP);
      break;
    // this is to get clang to stop complaining
    default:
      break;
    }
  }

  clean_jobs();
}

ssize_t write_all(int fd, const void *buf, size_t count) {
  size_t written = 0;

  do {
    ssize_t result = write(fd, (char *)buf + written, count - written);

    if (result == -1) {
      return -1;
    }
  } while (written < count);

  return (ssize_t)written;
}

#define STR(x) _STR(x)
#define _STR(x) #x
#define LOCATION __FILE__ ":" STR(__LINE__)

static void sigchld_handler(int sig) {
  (void)sig;

  int saved_errno = errno;

  int status;
  pid_t pid = waitpid(-1, &status, WNOHANG | WCONTINUED | WUNTRACED);

  if (pid == 0) {
    return;
  }

  if (pid == -1) {
    // cant use printf or strerror in a signal handler
    char *msg = LOCATION " fatal: waitpid failed in signal handler.\n";
    write_all(STDOUT_FILENO, msg, strlen(msg));
    _exit(-1);
  }

  Job *job = NULL;

  for (size_t i = 0; i < JOB_TABLE_SIZE; i++) {
    if (job_table[i].pid == pid) {
      job = job_table + i;
    }
  }

  if (job == NULL) {
    char *msg = LOCATION
        " fatal: job could not be found by pid. reached unreachable state.\n";
    write_all(STDOUT_FILENO, msg, strlen(msg));
    _exit(-1);
  }

  job->was_updated = true;

  if (WIFEXITED(status) || WIFSIGNALED(status)) {
    job->state = JOB_ENDED;
    job->wait_status = status;
  } else if (WIFCONTINUED(status)) {
    job->state = JOB_RUNNING;
  } else if (WIFSTOPPED(status)) {
    job->state = JOB_STOPPED;
  } else {
    char *msg = LOCATION " fatal: job is in unknown state.\n";
    write_all(STDOUT_FILENO, msg, strlen(msg));
    _exit(-1);
  }

  errno = saved_errno;
}

void sig_handler_init(void) {
  for (size_t i = 0; i < JOB_TABLE_SIZE; i++) {
    job_table[i].pid = -1;
  }

  (void)signal(SIGINT, SIG_IGN);
  (void)signal(SIGTSTP, SIG_IGN);
  (void)signal(SIGTTOU, SIG_IGN);

  struct sigaction sigchld_act = {0};
  sigchld_act.sa_handler = sigchld_handler;
  sigchld_act.sa_flags = SA_RESTART;
  sigemptyset(&sigchld_act.sa_mask);

  sigaction(SIGCHLD, &sigchld_act, NULL);
  
  sigemptyset(&oldset);
  sigprocmask(SIG_BLOCK, NULL, &oldset);

  int atexit_return = atexit(kill_all_children);
  assert(atexit_return == 0);

  if (interactive) {
    tty_fd = open("/dev/tty", O_RDWR, 0666);
    root_pid = getpid();

    if (tty_fd != -1 && isatty(tty_fd)) {
      setpgid(0, root_pid);
      tcsetpgrp(tty_fd, root_pid);
    } else {
      tty_fd = -1;
      error_f("rash: cannot access /dev/tty. Job control is unavailable.\n");
    }
  }
}

void reset_fg_process(void) {
  if (tty_fd != -1) {
    setpgid(0, root_pid);
    tcsetpgrp(tty_fd, root_pid);
  }
}

static void block_sigchld(void) {
  sigset_t set;
  sigemptyset(&set);
  
  sigaddset(&set, SIGCHLD);

  sigprocmask(SIG_BLOCK, &set, NULL);
}

static void unblock_sigchld(void) {
  sigprocmask(SIG_BLOCK, &oldset, NULL);
}

void clean_jobs(void) {
  block_sigchld();

  for (size_t i = 0; i < JOB_TABLE_SIZE; i++) {
    Job *job = job_table + i;

    if (!job->was_updated) {
      continue;
    }

    printf("[%zu]* PID: %d, ", i + 1, job->pid);

    if (job->state == JOB_ENDED) {
      if (WIFEXITED(job->wait_status)) {
        int exit_status = WEXITSTATUS(job->wait_status);

        if (exit_status == 0) {
          printf("Done\n");
        } else {
          printf("Exit %d\n", exit_status);
        }
      } else if (WIFSIGNALED(job->wait_status)) {
        int signal = WTERMSIG(job->wait_status);

        (void)fputs(strsignal(signal), stdout);

// WCOREDUMP is from POSIX.1-2024 so it's pretty new and might not be available
// everywhere.
#ifdef WCOREDUMP
        if (WCOREDUMP(job->wait_status)) {
          (void)fputs(" (core dumped)", stdout);
        }
#endif
      }

      putchar('\n');
      job->pid = -1;
      continue;
    }

    if (job->state == JOB_STOPPED) {
      printf("Stopped\n");
      continue;
    }

    if (job->state == JOB_RUNNING) {
      printf("Continued\n");
      continue;
    }
  }

  unblock_sigchld();
}

int register_job(pid_t pid, int state) {
  block_sigchld();

  Job *new_job = NULL;

  size_t i;
  for (i = 0; i < JOB_TABLE_SIZE; i++) {
    if (job_table[i].pid == -1) {
      new_job = job_table + i;
      break;
    }
  }

  if (new_job == NULL) {
    error("rash: job table is full.\n");
    return -1;
  }

  new_job->pid = pid;
  new_job->state = state;
  new_job->was_updated = false;

  printf("[%zu] PID: %d, State: %s\n", i + 1, new_job->pid,
         JOB_STATUSES[new_job->state]);

  unblock_sigchld();

  return (int)(i + 1);
}

Job *aquire_job(int id) {
  rash_assert(id <= JOB_TABLE_SIZE, "job id is too large");

  block_sigchld();

  if (id == -1) {
    Job *job = NULL;

    for (size_t i = 0; i < JOB_TABLE_SIZE; i++) {
      if (job_table[i].pid != -1) {
        job = job_table + i;
      }
    }

    return job;
  }

  Job *job = job_table + (size_t)id;

  if (job->pid == -1) {
    return NULL;
  }

  return job;
}

void release_job(void) {
  unblock_sigchld();
}

void print_jobs(void) {
  block_sigchld();

  for (size_t i = 0; i < JOB_TABLE_SIZE; i++) {
    printf("[%zu] PID: %d, State: %s\n", i + 1, job_table[i].pid,
           JOB_STATUSES[job_table[i].state]);
  }

  unblock_sigchld();
}

int wait_job(int id) {
  rash_assert(id <= JOB_TABLE_SIZE, "job id is too large");

  block_sigchld();

  if (job_table[id].state == JOB_ENDED) {
    return job
  }

  unblock_sigchld();
}
