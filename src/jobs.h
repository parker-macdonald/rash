#ifndef SIG_HANDLERS_H
#define SIG_HANDLERS_H

#include <signal.h>
#include <stdbool.h>

typedef enum {
  JOB_EXITED,
  JOB_STOPPED,
  JOB_RUNNING,
  NUM_JOB_STATUSES
} JobState;

// string versions for job statuses
extern const char *const JOB_STATUSES[NUM_JOB_STATUSES];

typedef struct job_t {
  pid_t pid;
  int id;
  JobState state;
  struct job_t *p_next;
} Job;

typedef struct {
  Job *root_job;
  Job *last_job;

  pid_t root_pid;
  int tty_fd;
} Jobs;

/**
 * @brief initializes all signal handlers used by rash
 */
void jobs_init(Jobs *self, int tty_fd);

void jobs_destroy(Jobs *self);

/**
 * @brief removes reaped children from the job list, good to call this
 * occasionally
 */
void jobs_clean(Jobs *self);

/**
 * @brief adds a job with the given pid to the job list
 * @param pid the pid of the job to add
 * @param state the state to give the job, i.e. JOB_STOPPED or JOB_RUNNING
 * @return the job id assigned to the new job
 */
int jobs_register(Jobs *self, pid_t pid, JobState state);

/**
 * @brief returns a job with the given id
 * @param id the id of the job to retrieve, you can pass -1 if you want to get
 * the last job in the job list
 * @return a pointer to the job with the id, or null if no job exists
 */
Job *jobs_get(Jobs *self, int id);

/**
 * @brief gets the pid of the job with the given id and removes it from the job
 * list. this is useful if you are bringing a job to the foreground since it is
 * no longer needed on the job list.
 * @param id the job id to get and remove
 * @return the pid of the found job, or 0 if no such job exists.
 */
pid_t jobs_get_pid_and_remove(Jobs *self, int id);

/**
 * @brief prints all jobs in the job list in a nicely formatted way. this is
 * used by the jobs command.
 */
void jobs_print(const Jobs *self);

void jobs_set_rash_to_foreground(const Jobs *self);

#endif
