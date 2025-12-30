#include "auto_complete.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../builtins/find_builtin.h"
#include "../vec_types.h"

void
get_file_matches(strings_t *matches, const char *word, size_t word_len) {
  DIR *dir;
  const char *basename;
  size_t basename_len;
  size_t last_slash = (size_t)(-1);

  for (size_t i = 0; i < word_len; i++) {
    if (word[i] == '/') {
      last_slash = i;
    }
  }

  // relative path
  if (last_slash == (size_t)-1) {
    dir = opendir(".");
    basename = word;
    basename_len = word_len;
  } else {
    // absolute path
    size_t path_len = last_slash;

    if (path_len == 0) {
      path_len++;
    }

    char *path = malloc(path_len + 1);
    if (path == NULL) {
      return;
    }

    memcpy(path, word, path_len);
    path[path_len] = '\0';

    dir = opendir(path);

    free(path);

    basename = (const char *)(word + last_slash + 1);
    basename_len = word_len - last_slash - 1;
  }

  if (dir == NULL) {
    return;
  }

  struct dirent *ent;

  while ((ent = readdir(dir))) {
    // must match hidden files explicitly
    if (ent->d_name[0] == '.' && basename[0] != '.') {
      continue;
    }
    if (strncmp(ent->d_name, basename, basename_len) != 0) {
      continue;
    }

    size_t file_len = strlen(ent->d_name);
    char *file_path = malloc(last_slash + file_len + 3);

    memcpy(file_path, word, last_slash + 1);
    memcpy(file_path + last_slash + 1, ent->d_name, file_len);

#ifdef _DIRENT_HAVE_D_TYPE
    if (ent->d_type == DT_DIR) {
      file_path[last_slash + file_len + 1] = '/';
    } else {
      file_path[last_slash + file_len + 1] = ' ';
    }
#else
    {
      struct stat sb = {0};
      if (fstatat(fd, ent->d_name, &sb, AT_SYMLINK_NOFOLLOW) == 0 &&
          S_ISDIR(sb.st_mode)) {
        file_path[last_slash + file_len + 1] = '/';
      } else {
        file_path[last_slash + file_len + 1] = ' ';
      }
    }
#endif

    file_path[last_slash + file_len + 2] = '\0';
    VECTOR_PUSH(*matches, file_path);
  }

  closedir(dir);
  return;
}

void
get_command_matches(strings_t *matches, const char *word, size_t word_len) {
  find_matching_builtins(word, word_len, matches);
  const char *path = getenv("PATH");

  if (path == NULL) {
    return;
  }

  char *path2 = strdup(path);

  char *path_part = strtok(path2, ":");

  while (path_part != NULL) {
    DIR *dir = opendir(path_part);
    int fd = dirfd(dir);
    assert(fd != -1);

    if (dir == NULL) {
      continue;
    }

    struct dirent *ent;

    while ((ent = readdir(dir))) {
      // must match hidden files explicitly
      if (ent->d_name[0] == '.' && word[0] != '.') {
        continue;
      }
      if (strncmp(ent->d_name, word, word_len) != 0) {
        continue;
      }
      // if file is not executable
      if (faccessat(fd, ent->d_name, X_OK, AT_EACCESS) != 0) {
        continue;
      }

      size_t file_len = strlen(ent->d_name);
      char *file = malloc(file_len + 2);

      memcpy(file, ent->d_name, file_len);
      file[file_len] = ' ';
      file[file_len + 1] = '\0';

      VECTOR_PUSH(*matches, file);
    }

    closedir(dir);
    path_part = strtok(NULL, ":");
  }

  free(path2);
}

