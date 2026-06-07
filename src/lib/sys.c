#include "lib/sys.h"
#include "lib/error.h"
#include <unistd.h>

void rash_kill(pid_t pid, int sig) {
  int res = kill(pid, sig);

  rash_panic(res == -1);
}

void rash_close(int fd) {
  int res = close(fd);

  rash_panic(res == -1);
}
