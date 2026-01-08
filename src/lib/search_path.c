#include "search_path.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "vector.h"

char *search_path(const char *file) {
  const char *path = getenv("PATH");
  if (path == NULL) {
    // musl does this same thing, not sure why /sbin or /usr/sbin aren't there.
    path = "/usr/local/bin:/bin:/usr/bin";
  }

  VECTOR(char) file_path;
  VECTOR_INIT(file_path);

  for (size_t i = 0;; i++) {
    if (path[i] == ':' || path[i] == '\0') {
      assert(file_path.length != 0);

      VECTOR_PUSH(file_path, '/');

      for (size_t j = 0; file[j] != '\0'; j++) {
        VECTOR_PUSH(file_path, file[j]);
      }

      VECTOR_PUSH(file_path, '\0');

      if (faccessat(AT_FDCWD, file_path.data, X_OK, AT_EACCESS) == 0) {
        return file_path.data;
      }

      if (path[i] == '\0') {
        break;
      }

      VECTOR_CLEAR(file_path);
      continue;
    }

    VECTOR_PUSH(file_path, path[i]);
  }

  VECTOR_DESTROY(file_path);
  return NULL;
}
