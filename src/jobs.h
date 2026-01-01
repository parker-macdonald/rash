#ifndef SIG_HANDLERS_H
#define SIG_HANDLERS_H
#include <signal.h>

#define JOB_EXITED 0
#define JOB_STOPPED 1
#define JOB_RUNNING 2
#define NUM_JOB_STATUSES 3

// string versions for job statuses
extern const char *const JOB_STATUSES[NUM_JOB_STATUSES];

typedef struct job {
  pid_t pid;
  int id;
  int state;
  struct job *p_next;
} job_t;

// pid of the current foreground process
extern volatile sig_atomic_t fg_pid;

// whether a sigtstp was recieved.
extern volatile sig_atomic_t recv_sigtstp;

// this is used for the line reader to print a ^C on sigint
extern volatile sig_atomic_t recv_sigint;

/**
 * @brief initializes all signal handlers used by rash
 */
void sig_handler_init(void);

/**
 * @brief removes reaped children from the job list, good to call this
 * occasionally
 */
void clean_jobs(void);

/**
 * @brief adds a job with the given pid to the job list
 * @param pid the pid of the job to add
 * @param state the state to give the job, i.e. JOB_STOPPED or JOB_RUNNING
 * @return the job id assigned to the new job
 */
int register_job(pid_t pid, int state);

/**
 * @brief returns a job with the given id
 * @param id the id of the job to retrieve, you can pass -1 if you want to get
 * the last job in the job list
 * @return a pointer to the job with the id, or null if no job exists
 */
job_t *get_job(int id);

/**
 * @brief gets the pid of the job with the given id and removes it from the job
 * list. this is useful if you are bringing a job to the foreground since it is
 * no longer needed on the job list.
 * @param id a pointer to the job id, when this function returns it puts the
 * real job id of the returned job in id, so if you pass -1 as your job id, id
 * will contain the job id of the last job in the job list on return.
 * @return the pid of the found job, or 0 if no such job exists.
 */
pid_t get_pid_and_remove(int *id);

/**
 * @brief prints all jobs in the job list in a nicely formatted way. this is
 * used by the jobs command.
 */
void print_jobs(void);

/**
 * @brief this function re-registers the sigint handler WITHOUT SA_RESTART set
 * in the flags, this is so you can preform an operation that must be aware a
 * sigint occured (such as reading a character). pretty much all of rash's code
 * does not handle the EINTR error, so calling this function, without later
 * calling restart_on_sigint will cause bugs.
 */
void dont_restart_on_sigint(void);

/**
 * @brief this function re-registers the sigint handler WITH SA_RESTART set
 * in the flags, this function must be called at some point after a call to
 * dont_restart_on_sigint since pretty much all of rash's code does not handle
 * the EINTR error. if you don't call this function after a call to
 * dont_restart_on_sigint, there WILL be bugs and other unintended consequences.
 */
void restart_on_sigint(void);

#endif
