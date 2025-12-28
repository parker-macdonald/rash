#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../builtins/find_builtin.h"
#include "../vec_types.h"
#include "modify_line.h"

#include "auto_complete.h"

static void get_file_matches(strings_t *matches, const char *word, size_t word_len) {
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
    char *file = malloc(file_len + 2);

    memcpy(file, ent->d_name, file_len);
#ifdef _DIRENT_HAVE_D_TYPE
    if (ent->d_type == DT_DIR) {
      file[file_len] = '/';
    } else {
      file[file_len] = ' ';
    }
#else
    {
      struct stat sb = {0};
      if (fstatat(fd, ent->d_name, &sb, AT_SYMLINK_NOFOLLOW) == 0 &&
          S_ISDIR(sb.st_mode)) {
        file[file_len] = '/';
      } else {
        file[file_len] = ' ';
      }
    }
#endif

    file[file_len + 1] = '\0';
    VECTOR_PUSH(*matches, file);
  }

  return;
}

static void get_command_matches(
    strings_t *matches, const char *word, size_t word_len
) {
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

size_t auto_complete(buf_t *line, size_t cursor_pos) {
  size_t word_start = cursor_pos - 1;

  for (; word_start > 0; word_start--) {
    if (line->data[word_start] == ' ') {
      word_start++;
      break;
    }
  }

  char *word = (char *)line->data + word_start;
  size_t word_len = cursor_pos - word_start;

  if (word_len == 0) {
    return 0;
  }

  strings_t matches;
  VECTOR_INIT(matches, 0);

  if (word_start == 0) {
    get_command_matches(&matches, word, word_len);
  } else {
    get_file_matches(&matches, word, word_len);
  }

  if (matches.length == 0) {
    return 0;
  }

  if (matches.length == 1) {
    size_t match_len = strlen(matches.data[0]);
    size_t bytes_written = match_len - word_len;

    line_insert_bulk(
        line, cursor_pos, (uint8_t *)matches.data[0] + word_len, bytes_written
    );

    free(matches.data[0]);
    VECTOR_DESTROY(matches);
    return bytes_written;
  }

  size_t i;

  for (i = 0;; i++) {
    for (size_t j = 0; j < matches.length - 1; j++) {
      if (matches.data[j][i] != matches.data[j + 1][i]) {
        goto leave;
      }
      if (matches.data[j][i] == '\0' || matches.data[j + 1][i] == '\0') {
        goto leave;
      }
    }
  }

leave: {
  size_t bytes_written = i - word_len;
  if (bytes_written != 0) {
    line_insert_bulk(
        line, cursor_pos, (uint8_t *)matches.data[0] + word_len, bytes_written
    );
  }

  for (size_t j = 0; j < matches.length; j++) {
    free(matches.data[j]);
  }
  VECTOR_DESTROY(matches);
  return bytes_written;
}
}
