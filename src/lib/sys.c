#include "lib/sys.h"

#include <signal.h>
#include <unistd.h>

#include "lib/error.h"

void rash_kill(pid_t pid, int sig) {
  if (kill(pid, sig) == -1) {
    rash_panic();
  }
}

void rash_close(int fd) {
  if (close(fd) == -1) {
    rash_panic();
  }
}
