#include "evaluate.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "argv0.h"
#include "execute.h"
#include "glob.h"
#include "lex.h"
#include "lib/error.h"
#include "lib/string.h"
#include "lib/vector.h"
#include "shell_vars.h"

static bool bad_syntax(const Token *const tokens) {
  if (tokens[0].type != ARGUMENT) {
    error_f("rash: invalid first token.\n");
    return true;
  }

  int stdout_count = 0;
  int stderr_count = 0;
  int stdin_count = 0;

  for (size_t i = 1; tokens[i].type != END; i++) {
    if (tokens[i].type == ARGUMENT) {
      continue;
    }

    if (tokens[i].type == STDIN_REDIR) {
      i++;
      if (tokens[i].type != ARGUMENT) {
        error_f("rash: expected filename after ‘<’.\n");
        return true;
      }

      stdin_count++;
      continue;
    }

    if (tokens[i].type == STDIN_REDIR_STRING) {
      i++;
      if (tokens[i].type != ARGUMENT) {
        error_f("rash: expected string after ‘<<<’.\n");
        return true;
      }

      stdin_count++;
      continue;
    }

    if (tokens[i].type == STDOUT_REDIR) {
      i++;
      if (tokens[i].type != ARGUMENT) {
        error_f("rash: expected filename after ‘>’.\n");
        return true;
      }

      stdout_count++;
      continue;
    }

    if (tokens[i].type == STDOUT_REDIR_APPEND) {
      i++;
      if (tokens[i].type != ARGUMENT) {
        error_f("rash: expected filename after ‘>>’.\n");
        return true;
      }

      stdout_count++;
      continue;
    }

    if (tokens[i].type == STDERR_REDIR) {
      i++;
      if (tokens[i].type != ARGUMENT) {
        error_f("rash: expected filename after ‘2>’.\n");
        return true;
      }

      stderr_count++;
      continue;
    }

    if (tokens[i].type == STDERR_REDIR_APPEND) {
      i++;
      if (tokens[i].type != ARGUMENT) {
        error_f("rash: expected filename after ‘2>>’.\n");
        return true;
      }

      stderr_count++;
      continue;
    }

    if (tokens[i].type == PIPE) {
      if (tokens[i + 1].type != ARGUMENT) {
        error_f("rash: expected command after ‘|’.\n");
        return true;
      }

      if (tokens[i - 1].type != ARGUMENT) {
        error_f("rash: bad placement of ‘|’.\n");
        return true;
      }

      stdout_count++;
    }

    if (stderr_count > 1) {
      error_f("rash: cannot redirect stderr more than once.\n");
      return true;
    }

    if (stdout_count > 1) {
      error_f("rash: cannot redirect stdout more than once.\n");
      return true;
    }

    if (stdin_count > 1) {
      error_f("rash: cannot redirect stdin more than once.\n");
      return true;
    }

    if (tokens[i].type == PIPE) {
      stdout_count = 0;
      stdin_count = 1;
      stderr_count = 0;

      i++;
    }

    if (tokens[i].type == LOGICAL_OR) {
      if (tokens[i + 1].type != ARGUMENT) {
        error_f("rash: expected command after ‘||’.\n");
        return true;
      }

      if (tokens[i - 1].type != ARGUMENT) {
        error_f("rash: bad placement of ‘||’.\n");
        return true;
      }

      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }

    if (tokens[i].type == LOGICAL_AND) {
      if (tokens[i + 1].type != ARGUMENT) {
        error_f("rash: expected command after ‘&&’.\n");
        return true;
      }

      if (tokens[i - 1].type != ARGUMENT) {
        error_f("rash: bad placement of ‘&&’.\n");
        return true;
      }

      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }

    if (tokens[i].type == SEMI) {
      if (tokens[i - 1].type != ARGUMENT) {
        error_f("rash: bad placement of ‘;’.\n");
        return true;
      }

      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }

    if (tokens[i].type == AMP) {
      if (tokens[i - 1].type != ARGUMENT) {
        error_f("rash: bad placement of ‘&’.\n");
        return true;
      }

      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }
  }

  return false;
}

static void set_exit_code_var(int code) {
  // 3 digit number (exit status is max of 255) + null terminator
  char status_str[3 + 1] = {0};
  (void)snprintf(status_str, sizeof(status_str), "%d", code & 0xff);
  var_set("?", status_str);
}

static char *evaluate_arg(const ArgumentPart *argument, bool *needs_globbing) {
  String buffer;
  VECTOR_INIT(buffer);

  for (size_t i = 0; argument[i].type != END_ARG; i++) {
    if (argument[i].type == STRING) {
      string_append(&buffer, argument[i].data);

      continue;
    }

    if (argument[i].type == ENV_EXPANSION) {
      const char *value = getenv(argument[i].data);

      if (value == NULL) {
        error_f("rash: environment variable ‘%s’ does not exist.\n",
                argument[i].data);
        goto error;
      }

      string_append(&buffer, value);

      continue;
    }

    if (argument[i].type == TILDE) {
      if (argument[i].data[0] == '\0') {
        char *home = getenv("HOME");
        if (home != NULL) {
          string_append(&buffer, home);
          continue;
        }
        error_f("cannot expand ‘~’, HOME is not set.\n");
        goto error;
      }

      struct passwd *pw = getpwnam(argument[i].data);
      if (pw == NULL || pw->pw_dir == NULL) {
        error_f("rash: cannot access user ‘%s’.\n", argument[i].data);
        goto error;
      }

      string_append(&buffer, pw->pw_dir);
      continue;
    }

    if (argument[i].type == VAR_EXPANSION) {
      const char *value = var_get(argument[i].data);

      if (value == NULL) {
        error_f("rash: shell variable ‘%s’ does not exist.\n",
                argument[i].data);
        goto error;
      }

      string_append(&buffer, value);

      continue;
    }

    if (argument[i].type == SUBSHELL) {
      char *argv[4] = {argv0, "-c", argument[i].data, NULL};

      int null_fd = open("/dev/null", O_RDWR);

      if (null_fd == -1) {
        error_f("rash: cannot open /dev/null: %s\n", strerror(errno));
        goto error;
      }

      int null_fd2 = dup(null_fd);

      if (null_fd2 == -1) {
        if (close(null_fd) == -1) {
          perror("rash: close");
        }
        perror("dup");
        goto error;
      }

      int fds[2];

      if (pipe(fds) == -1) {
        if (close(null_fd) == -1) {
          perror("rash: close");
        }
        if (close(null_fd2) == -1) {
          perror("rash: close");
        }
        goto error;
      }

      ExecutionContext ec = {.argv = argv,
                             .flags = EC_NO_WAIT,
                             .stderr_fd = null_fd,
                             .stdin_fd = null_fd2,
                             .stdout_fd = fds[1]};

      pid_t pid = execute(ec);

      if (pid == -1) {
        goto error;
      }

      char read_bytes[512];
      ssize_t nread;

      do {
        nread = read(fds[0], read_bytes, 512);

        if (nread == -1) {
          if (close(fds[0]) == -1) {
            perror("rash: close");
          }
          perror("rash: read");
          goto error;
        }

        for (ssize_t j = 0; j < nread; j++) {
          if (!iscntrl((int)read_bytes[j])) {
            VECTOR_PUSH(buffer, read_bytes[j]);
          }
        }
      } while (nread > 0);

      wait_process(pid);
      continue;
    }

    if (argument[i].type == GLOB_WILDCARD) {
      // this is a really dumb solution to this problem, but the line reader
      // assures that '\033' never be in the string, so it's not bad unless i
      // forget to strip out '\033' when i implement shell scripts. also if
      // futures globs besides the wildcard are added, this will need to be
      // reworked
      VECTOR_PUSH(buffer, '\033');
      *needs_globbing = true;
      continue;
    }
  }

  VECTOR_PUSH(buffer, '\0');

  return buffer.data;

error:
  VECTOR_DESTROY(buffer);
  return NULL;
}

int evaluate(const Token *tokens) {
  if (tokens[0].type == END) {
    return EXIT_SUCCESS;
  }

  if (bad_syntax(tokens)) {
    return EXIT_FAILURE;
  }

  StringList argv;
  VECTOR_INIT(argv);

  int last_status = -1;

  VECTOR(pid_t) wait_for_me = {0, 0, 0};
  // this doesn't compile on gcc :(
  // VECTOR_INIT(wait_for_me, 0);

  ExecutionContext ec = {NULL, -1, -1, -1, 0};

  for (size_t i = 0;; i++) {
    if (tokens[i].type == ARGUMENT) {
      bool needs_globbing = false;
      char *arg = evaluate_arg(tokens[i].data, &needs_globbing);

      if (arg == NULL) {
        goto error;
      }

      if (needs_globbing) {
        int args_added = glob(&argv, arg);
        if (args_added == 0) {
          for (size_t j = 0; arg[j] != '\0'; j++) {
            if (arg[j] == '\033') {
              arg[j] = '*';
            }
          }
          error_f("rash: nothing matched glob pattern ‘%s’.\n", arg);
          free(arg);
          goto error;
        }
        free(arg);
        if (args_added == -1) {
          goto error;
        }
        continue;
      }

      VECTOR_PUSH(argv, arg);
      continue;
    }

    if (tokens[i].type == STDIN_REDIR) {
      // this should've been taken care of with the call to bad syntax, but you
      // never know
      tokens++;
      assert(tokens[i].type == ARGUMENT);

      bool needs_globbing = false;
      char *filename = evaluate_arg(tokens[i].data, &needs_globbing);

      if (filename == NULL) {
        goto error;
      }
      if (needs_globbing) {
        error_f("rash: globing expression cannot be used as a filename.\n");
        free(filename);
        goto error;
      }

      int fd = open(filename, O_RDONLY);
      if (fd == -1) {
        error_f("rash: %s: %s\n", filename, strerror(errno));
        free(filename);
        goto error;
      }
      free(filename);

      ec.stdin_fd = fd;

      continue;
    }

    if (tokens[i].type == STDIN_REDIR_STRING) {
      // this should've been taken care of with the call to bad syntax, but you
      // never know
      tokens++;
      assert(tokens[i].type == ARGUMENT);

      int fds[2];
      if (pipe(fds) == -1) {
        perror("pipe");

        goto error;
      }

      bool needs_globbing = false;
      char *str = evaluate_arg(tokens[i].data, &needs_globbing);

      if (str == NULL) {
        goto error;
      }
      if (needs_globbing) {
        error_f(
            "rash: expected string after ‘<<<’, but found glob expression.\n");
        free(str);
        goto error;
      }

      size_t len = strlen(str);
      if (write(fds[1], str, len) == -1) {
        free(str);
        perror("write");

        goto error;
      }
      free(str);

      if (close(fds[1]) == -1) {
        perror("close");

        goto error;
      }

      ec.stdin_fd = fds[0];

      continue;
    }

    if (tokens[i].type == STDOUT_REDIR) {
      // this should've been taken care of with the call to bad syntax, but you
      // never know
      tokens++;
      assert(tokens[i].type == ARGUMENT);

      bool needs_globbing = false;
      char *filename = evaluate_arg(tokens[i].data, &needs_globbing);

      if (filename == NULL) {
        goto error;
      }
      if (needs_globbing) {
        error_f("rash: globing expression cannot be used as a filename.\n");
        free(filename);
        goto error;
      }

      int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        error_f("rash: %s: %s\n", filename, strerror(errno));

        free(filename);
        goto error;
      }
      free(filename);

      ec.stdout_fd = fd;

      continue;
    }

    if (tokens[i].type == STDOUT_REDIR_APPEND) {
      // this should've been taken care of with the call to bad syntax, but you
      // never know
      tokens++;
      assert(tokens[i].type == ARGUMENT);

      bool needs_globbing = false;
      char *filename = evaluate_arg(tokens[i].data, &needs_globbing);

      if (filename == NULL) {
        goto error;
      }
      if (needs_globbing) {
        error_f("rash: globing expression cannot be used as a filename.\n");
        free(filename);
        goto error;
      }

      int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        error_f("rash: %s: %s\n", filename, strerror(errno));

        free(filename);
        goto error;
      }
      free(filename);

      ec.stdout_fd = fd;

      continue;
    }

    if (tokens[i].type == STDERR_REDIR) {
      // this should've been taken care of with the call to bad syntax, but you
      // never know
      tokens++;
      assert(tokens[i].type == ARGUMENT);

      bool needs_globbing = false;
      char *filename = evaluate_arg(tokens[i].data, &needs_globbing);

      if (filename == NULL) {
        goto error;
      }
      if (needs_globbing) {
        error_f("rash: globing expression cannot be used as a filename.\n");
        free(filename);
        goto error;
      }

      int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        error_f("rash: %s: %s\n", filename, strerror(errno));

        free(filename);
        goto error;
      }
      free(filename);

      ec.stderr_fd = fd;

      continue;
    }

    if (tokens[i].type == STDERR_REDIR_APPEND) {
      // this should've been taken care of with the call to bad syntax, but you
      // never know
      tokens++;
      assert(tokens[i].type == ARGUMENT);

      bool needs_globbing = false;
      char *filename = evaluate_arg(tokens[i].data, &needs_globbing);

      if (filename == NULL) {
        goto error;
      }
      if (needs_globbing) {
        error_f("rash: globing expression cannot be used as a filename.\n");
        free(filename);
        goto error;
      }

      int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        error_f("rash: %s: %s\n", filename, strerror(errno));
        free(filename);

        goto error;
      }
      free(filename);

      ec.stderr_fd = fd;

      continue;
    }

    if (tokens[i].type == PIPE) {
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

      ec = (ExecutionContext){NULL, -1, fds[0], -1, 0};

      for (size_t j = 0; j < argv.length; j++) {
        free(argv.data[j]);
      }
      VECTOR_CLEAR(argv);
      continue;
    }

    if (tokens[i].type == AMP) {
      ec.flags = EC_BACKGROUND_JOB;
      continue;
    }

    if (argv.length > 0) {
      VECTOR_PUSH(argv, NULL);
      ec.argv = argv.data;
      last_status = execute(ec);
      set_exit_code_var(last_status);
      ec = (ExecutionContext){NULL, -1, -1, -1, 0};

      for (size_t j = 0; j < argv.length; j++) {
        free(argv.data[j]);
      }
      VECTOR_CLEAR(argv);

      for (size_t j = 0; j < wait_for_me.length; j++) {
        pid_t id = waitpid(wait_for_me.data[j], NULL, 0);
        // from my understanding, if waitpid fails, something in rash went wrong
        assert(id != -1);
      }
      VECTOR_CLEAR(wait_for_me);
    }

    if (tokens[i].type == LOGICAL_AND) {
      if (last_status != 0) {
        while ((tokens + 1)->type == ARGUMENT) {
          tokens++;
        }
      }

      continue;
    }

    if (tokens[i].type == LOGICAL_OR) {
      if (last_status == 0) {
        while ((tokens + 1)->type == ARGUMENT) {
          tokens++;
        }
      }

      continue;
    }

    if (tokens[i].type == SEMI) {
      continue;
    }

    if (tokens[i].type == END) {
      break;
    }
  }

  VECTOR_DESTROY(argv);
  VECTOR_DESTROY(wait_for_me);

  return last_status;

error:
  for (size_t i = 0; i < wait_for_me.length; i++) {
    pid_t id = waitpid(wait_for_me.data[i], NULL, 0);
    // from my understanding, if waitpid fails, something in rash went wrong
    assert(id != -1);
  }
  VECTOR_DESTROY(wait_for_me);
  for (size_t i = 0; i < argv.length; i++) {
    free(argv.data[i]);
  }
  VECTOR_DESTROY(argv);

  return EXIT_FAILURE;
}
