#include "evaluate.h"

#include <assert.h>
#include <errno.h>
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
  if (tokens[0].type != STRING) {
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

int evaluate(const token_t *tokens) {
  if (tokens[0].type == END) {
    return EXIT_SUCCESS;
  }

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

  execution_context ec = {NULL, -1, -1, -1, 0};

  for (;; tokens++) {
    if (tokens->type == STRING) {
      for (size_t i = 0; ((char *)(tokens->data))[i] != '\0'; i++) {
        VECTOR_PUSH(buffer, ((char *)(tokens->data))[i]);
      }
      VECTOR_PUSH(buffer, '\0');
      VECTOR_PUSH(argv, buffer.data);
      VECTOR_INIT(buffer);

      continue;
    }

    if (tokens->type == STDIN_REDIR) {
      // this should've been taken care of with the call to bad syntax, but you
      // never know
      assert((tokens + 1)->type == STRING);

      int fd = open((char *)(tokens + 1)->data, O_RDONLY);
      if (fd == -1) {
        fprintf(
            stderr,
            "rash: %s: %s\n",
            (char *)(tokens + 1)->data,
            strerror(errno)
        );

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
        fprintf(
            stderr,
            "rash: %s: %s\n",
            (char *)(tokens + 1)->data,
            strerror(errno)
        );

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
        fprintf(
            stderr,
            "rash: %s: %s\n",
            (char *)(tokens + 1)->data,
            strerror(errno)
        );

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
        fprintf(
            stderr,
            "rash: %s: %s\n",
            (char *)(tokens + 1)->data,
            strerror(errno)
        );

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
        fprintf(
            stderr,
            "rash: %s: %s\n",
            (char *)(tokens + 1)->data,
            strerror(errno)
        );

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
      if (last_status == -1) {
        goto error;
      }
      ec = (execution_context){NULL, -1, -1, -1, 0};

      for (size_t i = 0; i < argv.length; i++) {
        free(argv.data[i]);
      }
      VECTOR_CLEAR(argv);

      for (size_t i = 0; i < wait_for_me.length; i++) {
        pid_t id = waitpid(wait_for_me.data[i], NULL, 0);
        // from my understanding, if waitpid fails, something in rash went wrong
        assert(id != -1);
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
  for (size_t i = 0; i < wait_for_me.length; i++) {
    pid_t id = waitpid(wait_for_me.data[i], NULL, 0);
    // from my understanding, if waitpid fails, something in rash went wrong
    assert(id != -1);
  }
  VECTOR_DESTROY(wait_for_me);
  VECTOR_DESTROY(buffer);
  VECTOR_DESTROY(argv);

  return EXIT_FAILURE;
}
