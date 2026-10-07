#ifndef SYS_H
#define SYS_H

#include "lib/buffer.h"
#include "lib/error.h"

#include <signal.h>
#include <unistd.h>

#define signal_assert(sig, handler) rash_assert(signal((sig), (handler)) != SIG_ERR, "signal failed")
#define dup2_assert(fd1, fd2) rash_assert(dup2((fd1), (fd2)) != -1, "dup2 failed")
#define close_assert(fd) rash_assert(close(fd) != -1, "close failed")

Buffer getcwd_buffer(void);

// same as getcwd_buffer but $HOME is replaced by a tilde ~
Buffer get_pretty_cwd_buffer(void);

Buffer getlogin_buffer(void);

Buffer gethostname_buffer(void);

#endif
