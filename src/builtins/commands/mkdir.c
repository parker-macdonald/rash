#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "builtins/builtins.h"
#include "lib/buffer.h"
#include "lib/error.h"

static const char *const MKDIR_HELP =
"Usage: mkdir [-p] [-c] [-m mode] directory ...\n"
"Create the directories listed in the arguments. By default these directories\n"
"will have the mode rwxrwxrwx (0777), but this can be overwritten with the `-m`\n"
"flag. If the `-p` flag is specified extra intermediate directories will be\n"
"created so that the entire path will be valid and errors about directories\n"
"already existing won't be reported. If the `-c` flag is specified the last\n"
"directory in the list will be set to the current working directory (i.e. cd\n"
"dir) so long as there aren't any errors in the creation of the last directory.";

static int parse_mode(const char *str, mode_t *dest) {
  if (*str != '0') {
    error("mkdir: mode must start with `0` (i.e. 0777).\n");
    return -1;
  }

  str++;

  mode_t result = 0;

  for (int i = 0; i < 3; i++) {
    if (!(*str >= '0' && *str <= '7')) {
      error_f("mkdir: expected octal digit (0-7) found `%c`.\n", *str);
      return -1;
    }

    result *= 8;
    result += (mode_t)(*str - '0');

    str++;
  }

  if (*str != '\0') {
    error_f("mkdir: expected end of mode, found `%c`.\n", *str);
    return -1;
  }

  *dest = result;
  return 0;
}

static int mkdir_good(const char *path, mode_t mode, bool parent_flag) {
  if (!parent_flag) {
    return mkdir(path, mode);
  }

  Buffer path_buf = buffer_from_cstr(path);
  size_t i = 1;

  while (1) {
    i = buffer_find_from_offset(&path_buf, '/', i);

    if (i == (size_t)-1) {
      int status = mkdir(buffer_cstr(&path_buf), mode);

      if (status == -1 && errno != EEXIST) {
        goto error;
      }
      break;
    }

    Buffer path_part = buffer_slice(&path_buf, 0, i);

    int status = mkdir(buffer_cstr(&path_part), mode);

    buffer_destroy(&path_part);

    if (status == -1 && errno != EEXIST) {
      goto error;
    }

    i++;
  }

  buffer_destroy(&path_buf);
  return 0;

error:
  buffer_destroy(&path_buf);
  return -1;
}

int builtin_mkdir(char **argv) {
  argv++;

  if (*argv == NULL) {
    puts(MKDIR_HELP);
    return EXIT_FAILURE;
  }

  if (strcmp(*argv, "--help") == 0) {
    puts(MKDIR_HELP);
    return EXIT_SUCCESS;
  }

  bool cd_flag = false;
  bool parent_flag = false;
  mode_t mode = 0777;

  while (1) {
    if (*argv == NULL) {
      error("mkdir: expected directory name.\n");
      return EXIT_FAILURE;
    }

    if (*argv[0] != '-') {
      break;
    }

    if (strcmp(*argv, "-c") == 0) {
      cd_flag = true;
    }

    else if (strcmp(*argv, "-p") == 0) {
      parent_flag = true;
    }

    else if (strcmp(*argv, "-m") == 0) {
      argv++;
      if (*argv == NULL) {
        error("mkdir: expected mode (i.e. 0777) after `-m`.\n");
        return EXIT_FAILURE;
      }

      if (parse_mode(*argv, &mode)) {
        return EXIT_FAILURE;
      }
    }

    argv++;
  }

  bool last_failed = false;

  while (*argv != NULL) {
    last_failed = false;
    if (mkdir_good(*argv, mode, parent_flag) == -1) {
      error_f("mkdir: %s: %s\n", *argv, strerror(errno));
      last_failed = true;
    }

    argv++;
  }

  argv--;

  if (!last_failed && cd_flag) {
    if (chdir(*argv) == -1) {
      error_f("mkdir: cd: %s: %s\n", *argv, strerror(errno));

      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
