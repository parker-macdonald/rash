#include "jobs.h"

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static_assert(sizeof(sig_atomic_t) == sizeof(pid_t),
              "size of sig_atomic_t differs from pid_t. what the hell are "
              "you compiling this on?");

const char *const JOB_STATUSES[NUM_JOB_STATUSES] = {"Exited", "Stopped",
                                                    "Running"};

static struct sigaction sigint_act;
static struct sigaction sigtstp_act;
static struct sigaction sigchld_act;

static job_t *root_job = NULL;
static job_t *last_job = NULL;

// pid of the current foreground process
volatile sig_atomic_t fg_pid = 0;

// whether a sigtstp was recieved.
volatile sig_atomic_t recv_sigtstp = 0;

static void sigint_handler(int sig) {
  if (fg_pid != 0) {
    kill((pid_t)fg_pid, sig);
    fg_pid = 0;
  }

  // re-register sigint handler
  sigaction(SIGINT, &sigint_act, NULL);
}

static void sigtstp_handler(int sig) {
  if (fg_pid != 0) {
    kill((pid_t)fg_pid, sig);
    recv_sigtstp = 1;
  }

  // re-register sigint handler
  sigaction(SIGTSTP, &sigtstp_act, NULL);
}

// traversing a linked list in a signal handler will probably cause crazy
// segfaults if the list is being modified when the signal handler is called.
// should probably fix this...
static void sigchld_handler(int sig) {
  (void)sig;

  int status;
  pid_t pid = waitpid(-1, &status, WNOHANG);

  if (pid == 0) {
    return;
  }

  for (job_t *current = root_job; current != NULL; current = current->p_next) {
    if (pid == current->pid) {
      current->state = JOB_EXITED;
      break;
    }
  }

  // re-register sigint handler
  sigaction(SIGCHLD, &sigchld_act, NULL);
}

void sig_handler_init(void) {
  sigint_act.sa_handler = sigint_handler;
  sigint_act.sa_flags = 0;
  sigemptyset(&sigint_act.sa_mask);

  sigtstp_act.sa_handler = sigtstp_handler;
  sigtstp_act.sa_flags = 0;
  sigemptyset(&sigtstp_act.sa_mask);

  sigchld_act.sa_handler = sigchld_handler;
  sigchld_act.sa_flags = SA_NOCLDSTOP;
  sigemptyset(&sigchld_act.sa_mask);

  // Set up SIGINT handler using sigaction
  sigaction(SIGINT, &sigint_act, NULL);

  // Set up SIGTSTP handler using sigaction
  sigaction(SIGTSTP, &sigtstp_act, NULL);

  // Set up SIGTSTP handler using sigaction
  sigaction(SIGCHLD, &sigchld_act, NULL);
}

void clean_jobs(void) {
  job_t *current;
  job_t *prev = NULL;

  for (current = root_job; current != NULL;) {
    if (current->state == JOB_EXITED) {
      if (prev != NULL) {
        prev->p_next = current->p_next;
      } else {
        root_job = current->p_next;
      }

      if (current->p_next == NULL) {
        last_job = prev;
      }

      job_t *temp = current;
      current = current->p_next;

      printf("[%d] PID: %d, State: %s\n", temp->id, temp->pid,
             JOB_STATUSES[temp->state]);
      free(temp);

      continue;
    }

    prev = current;
    current = current->p_next;
  }
}

int register_stopped_job(pid_t pid) {
  job_t *new_job = malloc(sizeof(job_t));

  new_job->p_next = NULL;

  new_job->pid = pid;

  new_job->state = JOB_STOPPED;

  if (last_job == NULL) {
    new_job->id = 1;
    root_job = new_job;
  } else {
    new_job->id = last_job->id + 1;

    last_job->p_next = new_job;
  }

  last_job = new_job;

  printf("\n[%d] PID: %d, State: %s\n", new_job->id, new_job->pid,
         JOB_STATUSES[new_job->state]);

  return new_job->id;
}

job_t *get_job(int id) {
  if (root_job == NULL) {
    return NULL;
  }

  job_t *current;

  if (id == -1) {
    return last_job;
  }

  for (current = root_job; current != NULL; current = current->p_next) {
    if (current->id == id) {
      return current;
    }
  }

  return NULL;
}

pid_t get_pid_and_remove(int *id) {
  if (root_job == NULL) {
    return 0;
  }

  job_t *current;
  job_t *prev = NULL;

  if (*id == -1) {
    for (current = root_job;; current = current->p_next) {
      if (current->p_next == NULL) {
        pid_t pid = current->pid;
        *id = current->id;
        last_job = prev;
        if (prev != NULL) {
          prev->p_next = NULL;
        } else {
          root_job = NULL;
        }

        free(current);

        return pid;
      }
      prev = current;
    }

    // this should never be reached
    assert(0);
  }

  for (current = root_job; current != NULL; current = current->p_next) {
    if (current->id == *id) {
      pid_t pid = current->pid;

      if (prev != NULL) {
        prev->p_next = current->p_next;
      } else {
        root_job = current->p_next;
      }

      if (current->p_next == NULL) {
        last_job = prev;
      }

      free(current);
      return pid;
    }

    prev = current;
  }

  return 0;
}

void print_jobs(void) {
  for (job_t *current = root_job; current != NULL; current = current->p_next) {
    printf("[%d] PID: %d, State: %s\n", current->id, current->pid,
           JOB_STATUSES[current->state]);
  }
}
