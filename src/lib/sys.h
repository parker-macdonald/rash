#ifndef SYS_H
#define SYS_H

#include <signal.h>
#include "lib/buffer.h"

void rash_kill(pid_t pid, int sig);

void rash_close(int fd);

Buffer getcwd_buffer(void);

// same as getcwd_buffer but $HOME is replaced by a tilde ~
Buffer get_pretty_cwd_buffer(void);

Buffer getlogin_buffer(void);

Buffer gethostname_buffer(void);

#endif
