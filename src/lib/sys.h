#ifndef SYS_H
#define SYS_H

#include <signal.h>

void rash_kill(pid_t pid, int sig);

void rash_close(int fd);

#endif
