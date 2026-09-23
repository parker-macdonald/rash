#include "jobs.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lib/error.h"
#include "rash.h"

const char *const JOB_STATUSES[NUM_JOB_STATUSES] = {
    "Exited", "Stopped", "Running"
};

static void kill_all_children(void) {
  Jobs *state = &rash_instance_get()->jobs;

  Job *current = state->root_job;

  while (current != NULL) {
    switch (current->state) {
      case JOB_STOPPED:
        kill(current->pid, SIGCONT);
        kill(current->pid, SIGHUP);
        break;
      case JOB_RUNNING:
        kill(current->pid, SIGHUP);
        break;
      // this is to get clang to stop complaining
      default:
        break;
    }

    current = current->p_next;
  }

  jobs_clean(state);
}

static void sigint_handler(int sig) {
  (void)sig;
}

void jobs_init(Jobs *self, int tty_fd) {
  self->root_pid = getpid();
  setpgid(0, self->root_pid);

  self->root_job = NULL;
  self->last_job = NULL;

  struct sigaction sigint_act;
  sigint_act.sa_handler = sigint_handler;
  sigint_act.sa_flags = 0;

  rash_assert(sigemptyset(&sigint_act.sa_mask) == 0, "sigemptyset failed");
  rash_assert(sigaction(SIGINT, &sigint_act, NULL) == 0, "sigaction failed");

  rash_assert(signal(SIGTSTP, SIG_IGN) != SIG_ERR, "signal failed");
  rash_assert(signal(SIGTTOU, SIG_IGN) != SIG_ERR, "signal failed");

  if (tty_fd != -1) {
    // set ourselves as the foreground process
    tcsetpgrp(self->tty_fd, self->root_pid);
  }

  // this function uses the global instance of Jobs, so we call it after everything is properly setup
  rash_assert(atexit(kill_all_children) == 0, "atexit failed");
}

void jobs_set_rash_to_foreground(const Jobs *self) {
  if (self->tty_fd != -1) {
    setpgid(0, self->root_pid);
    tcsetpgrp(self->tty_fd, self->root_pid);
  }
}

void jobs_clean(Jobs *self) {
  Job *current = self->root_job;
  Job *prev = NULL;

  while (current != NULL) {
    int status;
    pid_t pid = waitpid(current->pid, &status, WNOHANG | WUNTRACED);

    if (current->pid == pid) {
      printf("[%d]* PID: %d, ", current->id, pid);

      if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);

        if (exit_status == 0) {
          printf("Done\n");
        } else {
          printf("Exit %d\n", exit_status);
        }
      } else if (WIFSIGNALED(status)) {
        int signal = WTERMSIG(status);

        (void)fputs(strsignal(signal), stdout);

// WCOREDUMP is from POSIX.1-2024 so it's pretty new and might not be available
// everywhere.
#ifdef WCOREDUMP
        if (WCOREDUMP(status)) {
          (void)fputs(" (core dumped)", stdout);
        }
#endif

        putchar('\n');
      } else if (WIFSTOPPED(status)) {
        printf("Stopped\n");

        current->state = JOB_STOPPED;
        prev = current;
        current = current->p_next;
        continue;
      }

      if (prev != NULL) {
        prev->p_next = current->p_next;
      } else {
        self->root_job = current->p_next;
      }

      if (current->p_next == NULL) {
        self->last_job = prev;
      }

      Job *temp = current;
      current = current->p_next;

      free(temp);
      continue;
    }

    prev = current;
    current = current->p_next;
  }
}

int jobs_register(Jobs *self, pid_t pid, JobState state) {
  Job *new_job = malloc(sizeof(Job));

  new_job->p_next = NULL;

  new_job->pid = pid;

  new_job->state = state;

  if (self->last_job == NULL) {
    new_job->id = 1;
    self->root_job = new_job;
  } else {
    new_job->id = self->last_job->id + 1;

    self->last_job->p_next = new_job;
  }

  self->last_job = new_job;

  printf(
      "[%d] PID: %d, State: %s\n",
      new_job->id,
      new_job->pid,
      JOB_STATUSES[new_job->state]
  );

  return new_job->id;
}

Job *jobs_get(Jobs *self, int id) {
  if (self->root_job == NULL) {
    return NULL;
  }

  if (id == -1) {
    return self->last_job;
  }

  for (Job *current = self->root_job; current != NULL; current = current->p_next) {
    if (current->id == id) {
      return current;
    }
  }

  return NULL;
}

pid_t jobs_get_pid_and_remove(Jobs *self, int id) {
  if (self->root_job == NULL) {
    return 0;
  }


  if (id == -1) {
    Job *current = self->root_job;
    Job *prev = NULL;
    
    while(1) {
      if (current->p_next == NULL) {
        pid_t pid = current->pid;

        self->last_job = prev;
        if (prev != NULL) {
          prev->p_next = NULL;
        } else {
          self->root_job = NULL;
        }

        free(current);

        return pid;
      }

      prev = current;
      current = current->p_next;
    }

    unreachable();
  }

  Job *current = self->root_job;
  Job *prev = NULL;

  while (current != NULL) {
    if (current->id == id) {
      pid_t pid = current->pid;

      if (prev != NULL) {
        prev->p_next = current->p_next;
      } else {
        self->root_job = current->p_next;
      }

      if (current->p_next == NULL) {
        self->last_job = prev;
      }

      free(current);

      return pid;
    }

    prev = current;
    current = current->p_next;
  }

  return 0;
}

void jobs_print(const Jobs *self) {
  for (Job *current = self->root_job; current != NULL; current = current->p_next) {
    printf(
        "[%d] PID: %d, State: %s\n",
        current->id,
        current->pid,
        JOB_STATUSES[current->state]
    );
  }
}

void jobs_destroy(Jobs *self) {
  (void)self;
}
