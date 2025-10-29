#include "evaluate.h"

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../vector.h"
#include "execute.h"
#include "lex.h"

static bool bad_syntax(const token_t *const tokens) {
  if (!(tokens[0].type & ARGUMENT_TOKENS)) {
    fprintf(stderr, "rash: invalid first token.\n");
    return true;
  }

  int stdout_count = 0;
  int stderr_count = 0;
  int stdin_count = 0;

  for (size_t i = 1; tokens[i].type != END; i++) {
    if (tokens[i].type == STRING) {
      continue;
    }

    if (tokens[i].type == STDIN_REDIR) {
      if (tokens[i + 1].type != STRING) {
        fprintf(stderr, "rash: expected file name after ‘<’.\n");
        return true;
      }

      i++;
      stdin_count++;
    }

    if (tokens[i].type == STDIN_REDIR_STRING) {
      if (tokens[i + 1].type != STRING) {
        fprintf(stderr, "rash: expected string after ‘<<<’.\n");
        return true;
      }

      i++;
      stdin_count++;
    }

    if (tokens[i].type == STDOUT_REDIR) {
      if (tokens[i + 1].type != STRING) {
        fprintf(stderr, "rash: expected string after ‘>’.\n");
        return true;
      }

      i++;
      stdout_count++;
    }

    if (tokens[i].type == STDOUT_REDIR_APPEND) {
      if (tokens[i + 1].type != STRING) {
        fprintf(stderr, "rash: expected string after ‘>>’.\n");
        return true;
      }

      i++;
      stdout_count++;
    }

    if (tokens[i].type == STDERR_REDIR) {
      if (tokens[i + 1].type != STRING) {
        fprintf(stderr, "rash: expected string after ‘2>’.\n");
        return true;
      }

      i++;
      stderr_count++;
    }

    if (tokens[i].type == STDERR_REDIR_APPEND) {
      if (tokens[i + 1].type != STRING) {
        fprintf(stderr, "rash: expected string after ‘2>>’.\n");
        return true;
      }

      i++;
      stderr_count++;
    }

    if (tokens[i].type == PIPE) {
      if (tokens[i + 1].type != STRING) {
        fprintf(stderr, "rash: expected command after ‘|’.\n");
        return true;
      }

      if (tokens[i - 1].type != STRING) {
        fprintf(stderr, "rash: bad placement of ‘|’.\n");
        return true;
      }

      stdout_count++;
    }

    if (stderr_count > 1) {
      fprintf(stderr, "rash: cannot redirect stderr more than once.\n");
      return true;
    }

    if (stdout_count > 1) {
      fprintf(stderr, "rash: cannot redirect stdout more than once.\n");
      return true;
    }

    if (stdin_count > 1) {
      fprintf(stderr, "rash: cannot redirect stdin more than once.\n");
      return true;
    }

    if (tokens[i].type == PIPE) {
      stdout_count = 0;
      stdin_count = 1;
      stderr_count = 0;

      i++;
    }

    if (tokens[i].type == LOGICAL_OR) {
      if (tokens[i + 1].type != STRING) {
        fprintf(stderr, "rash: expected command after ‘||’.\n");
        return true;
      }

      if (tokens[i - 1].type != STRING) {
        fprintf(stderr, "rash: bad placement of ‘||’.\n");
        return true;
      }

      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }

    if (tokens[i].type == LOGICAL_AND) {
      if (tokens[i + 1].type != STRING) {
        fprintf(stderr, "rash: expected command after ‘&&’.\n");
        return true;
      }

      if (tokens[i - 1].type != STRING) {
        fprintf(stderr, "rash: bad placement of ‘&&’.\n");
        return true;
      }

      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }

    if (tokens[i].type == SEMI) {
      if (tokens[i - 1].type != STRING) {
        fprintf(stderr, "rash: bad placement of ‘;’.\n");
        return true;
      }

      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }

    if (tokens[i].type == AMP) {
      if (tokens[i - 1].type != STRING) {
        fprintf(stderr, "rash: bad placement of ‘&’.\n");
        return true;
      }

      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }
  }

  return false;
}

typedef VECTOR(char *) argv_t;

// modified Krauss's wildcard matching algorithm
bool match(char *str, char *pattern) {
  if (str[0] == '.' && pattern[0] != '.') {
    return false;
  }

  char *after_last_wild = NULL;
  // Location after last "*", if we've encountered one
  char t, w;
  // Walk the text strings one character at a time.
  while (1) {
    t = *str;
    w = *pattern;
    // How do you match a unique text string?
    if (!t || t == '/') {
      // Easy: unique up on it!
      if (!w || w == '/') {
        return true; // "x" matches "x"
      } else if (w == '\033') {
        pattern++;
        continue; // "x*" matches "x" or "xy"
      }
      return false; // "x" doesn't match "xy"
    } else {
      // How do you match a tame text string?
      if (t != w) {
        // The tame way: unique up on it!
        if (w == '\033') {
          after_last_wild = ++pattern;
          continue; // "*y" matches "xy"
        } else if (after_last_wild) {
          pattern = after_last_wild;
          w = *pattern;

          if (!w || w == '/') {
            return true; // "*" matches "x"
          } else if (t == w) {
            pattern++;
          }
          str++;
          continue; // "*sip*" matches "mississippi"
        } else {
          return false; // "x" doesn't match "y"
        }
      }
    }
    str++;
    pattern++;
  }
  return true;
}

int glob_recurse(argv_t *argv, char *pattern, char* path, size_t path_len) {
  DIR* dir = opendir(path);

  if (dir == NULL) {
    return 0;
  }

  struct dirent *ent;

  bool end = false;
  char* new_pattern = pattern;
  for (; ; new_pattern++) {
    if (*new_pattern == '/') {
      new_pattern++;
      if (*new_pattern == '\0') {
        end = true;
      }
      break;
    }

    if (*new_pattern == '\0') {
      end = true;
      break;
    }
  }

  while ((ent = readdir(dir)) != NULL) {
    if (match(ent->d_name, pattern)) {
      if (end) {
        VECTOR_PUSH(*argv, strdup(ent->d_name));
        continue;
      }

      size_t ent_len = strlen(ent->d_name);

      char* new_path = malloc(path_len + ent_len + 2);
      memcpy(new_path, path, path_len);
      new_path[path_len] = '/';
      memcpy(new_path + path_len + 1, ent->d_name, ent_len);
      new_path[path_len + ent_len + 1] = '\0';

      glob_recurse(argv, new_pattern, new_path, path_len + ent_len + 1);

      free(new_path);
    }
  }

  closedir(dir);

  return 0;
}

int glob(argv_t *argv, char *pattern) {
  char* path;

  if (pattern[0] == '/') {
    pattern++;
    path = "/";
  } else {
    path = ".";
  }

  return glob_recurse(argv, pattern, path, 1);
}

int evaluate(const token_t *tokens) {
  if (bad_syntax(tokens)) {
    return EXIT_FAILURE;
  }

  argv_t argv;
  VECTOR_INIT(argv);

  VECTOR(char) buffer;
  VECTOR_INIT(buffer);

  int last_status = -1;

  VECTOR(pid_t) wait_for_me = {0, 0, 0};
  // this doesn't compile on gcc :(
  // VECTOR_INIT(wait_for_me, 0);

  bool needs_globbing = false;

  execution_context ec = {NULL, -1, -1, -1, 0};

  for (;; tokens++) {
    if (tokens->type == STRING) {
      for (size_t i = 0; ((char *)(tokens->data))[i] != '\0'; i++) {
        VECTOR_PUSH(buffer, ((char *)(tokens->data))[i]);
      }

      continue;
    }

    if (tokens->type == ENV_EXPANSION) {
      char *value = getenv((char *)tokens->data);

      if (value == NULL) {
        fprintf(
            stderr,
            "rash: environment variable ‘%s’ does not exist.\n",
            (char *)tokens->data
        );
        goto error;
      }

      for (size_t i = 0; value[i] != '\0'; i++) {
        VECTOR_PUSH(buffer, value[i]);
      }

      continue;
    }

    if (tokens->type == GLOB_WILDCARD) {
      VECTOR_PUSH(buffer, '\033');
      needs_globbing = true;
      continue;
    }

    if (tokens->type == END_ARG) {
      VECTOR_PUSH(buffer, '\0');
      if (needs_globbing) {
        glob(&argv, buffer.data);
        VECTOR_DESTROY(buffer);
      } else {
        VECTOR_PUSH(argv, buffer.data);
      }
      VECTOR_INIT(buffer);

      continue;
    }

    if (tokens->type == STDIN_REDIR) {
      // this should've been taken care of with the call to bad syntax, but you
      // never know
      assert((tokens + 1)->type == STRING);

      int fd = open((char *)(tokens + 1)->data, O_RDONLY);
      if (fd == -1) {
        fprintf(stderr, "rash: ");
        perror((char *)(tokens + 1)->data);

        goto error;
      }

      ec.stdin_fd = fd;

      tokens++;
      continue;
    }

    if (tokens->type == STDIN_REDIR_STRING) {
      assert((tokens + 1)->type == STRING);

      int fds[2];
      if (pipe(fds) == -1) {
        perror("pipe");

        goto error;
      }

      size_t len = strlen((char *)(tokens + 1)->data);
      if (write(fds[1], (tokens + 1)->data, len) == -1) {
        perror("write");

        goto error;
      }

      if (close(fds[1]) == -1) {
        perror("close");

        goto error;
      }

      ec.stdin_fd = fds[0];

      tokens++;
      continue;
    }

    if (tokens->type == STDOUT_REDIR) {
      assert((tokens + 1)->type == STRING);

      int fd = open((tokens + 1)->data, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        fprintf(stderr, "rash: ");
        perror((char *)(tokens + 1)->data);

        goto error;
      }

      ec.stdout_fd = fd;

      tokens++;
      continue;
    }

    if (tokens->type == STDOUT_REDIR_APPEND) {
      assert((tokens + 1)->type == STRING);

      int fd = open((tokens + 1)->data, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        fprintf(stderr, "rash: ");
        perror((char *)(tokens + 1)->data);

        goto error;
      }

      ec.stdout_fd = fd;

      tokens++;
      continue;
    }

    if (tokens->type == STDERR_REDIR) {
      assert((tokens + 1)->type == STRING);

      int fd = open((tokens + 1)->data, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        fprintf(stderr, "rash: ");
        perror((char *)(tokens + 1)->data);

        goto error;
      }

      ec.stderr_fd = fd;

      tokens++;
      continue;
    }

    if (tokens->type == STDERR_REDIR_APPEND) {
      assert((tokens + 1)->type == STRING);

      int fd = open((tokens + 1)->data, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        fprintf(stderr, "rash: ");
        perror((char *)(tokens + 1)->data);

        goto error;
      }

      ec.stderr_fd = fd;

      tokens++;
      continue;
    }

    if (tokens->type == PIPE) {
      VECTOR_PUSH(argv, NULL);

      int fds[2];
      if (pipe(fds) == -1) {
        perror("pipe");
        goto error;
      }

      ec.stdout_fd = fds[1];
      ec.argv = argv.data;
      ec.flags = EC_NO_WAIT;
      pid_t pid = execute(ec);
      if (pid == -1) {
        goto error;
      }
      VECTOR_PUSH(wait_for_me, pid);

      ec = (execution_context){NULL, -1, fds[0], -1, 0};

      VECTOR_CLEAR(argv);
      continue;
    }

    if (tokens->type == AMP) {
      ec.flags = EC_BACKGROUND_JOB;
      continue;
    }

    if (argv.length > 0) {
      VECTOR_PUSH(argv, NULL);
      ec.argv = argv.data;
      last_status = execute(ec);
      ec = (execution_context){NULL, -1, -1, -1, 0};

      for (size_t i = 0; i < argv.length; i++) {
        free(argv.data[i]);
      }
      VECTOR_CLEAR(argv);

      for (size_t i = 0; i < wait_for_me.length; i++) {
        (void)waitpid(wait_for_me.data[i], NULL, 0);
      }
      VECTOR_CLEAR(wait_for_me);
    }

    if (tokens->type == LOGICAL_AND) {
      if (last_status != 0) {
        while ((tokens + 1)->type == STRING) {
          tokens++;
        }
      }

      continue;
    }

    if (tokens->type == LOGICAL_OR) {
      if (last_status == 0) {
        while ((tokens + 1)->type == STRING) {
          tokens++;
        }
      }

      continue;
    }

    if (tokens->type == SEMI) {
      continue;
    }

    if (tokens->type == END) {
      break;
    }
  }

  VECTOR_DESTROY(argv);
  VECTOR_DESTROY(buffer);
  VECTOR_DESTROY(wait_for_me);

  return last_status;

error:
  VECTOR_DESTROY(wait_for_me);
  VECTOR_DESTROY(buffer);
  VECTOR_DESTROY(argv);

  return EXIT_FAILURE;
}
