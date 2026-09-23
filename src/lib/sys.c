#include "lib/sys.h"

#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "lib/buffer.h"
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

Buffer getcwd_buffer(void) {
  Buffer buffer = buffer_create(16);

  char *cwd = getcwd(buffer.char_ptr, buffer._capacity);

  while (cwd == NULL) {
    if (errno != ERANGE) {
      rash_panic();
    }

    buffer_grow_to(&buffer, buffer._capacity * 2);

    cwd = getcwd(buffer.char_ptr, buffer._capacity);
  }

  buffer.length = strlen(buffer.char_ptr);

  return buffer;
}

Buffer get_pretty_cwd_buffer(void) {
  Buffer cwd = getcwd_buffer();

  const char *home = getenv("HOME");

  if (home != NULL && buffer_starts_with_cstr(&cwd, home)) {
    size_t home_len = strlen(home);

    buffer_remove_n(&cwd, 0, home_len);
    buffer_insert(&cwd, 0, '~');
  }

  return cwd;
}

Buffer getlogin_buffer(void) {
  struct passwd *p = getpwuid(geteuid());

  if (p == NULL) {
    return buffer_create(0);
  }

  return buffer_from_cstr(p->pw_name);
}

Buffer gethostname_buffer(void) {
  struct utsname name;
  // this function only fails if name is an invalid pointer, and &name isn't
  (void)uname(&name);

  return buffer_from_cstr(name.nodename);
}
