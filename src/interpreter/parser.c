#include "parser.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../vector.h"
#include "execute.h"
#include "lex.h"

bool bad_syntax(const token_t *const tokens) {
  if (tokens[0].type != STRING) {
    fprintf(stderr, "rash: expected string as first token.\n");
    return true;
  }

  for (size_t i = 1; tokens[i].type != END; i++) {
    if (tokens[i].type == STRING) {
      continue;
    }

    if (tokens[i].type == STDIN_REDIR && tokens[i + 1].type != STRING) {
      fprintf(stderr, "rash: expected file name after ‘<’.\n");
      return true;
    }

    if (tokens[i].type == STDIN_REDIR_STRING && tokens[i + 1].type != STRING) {
      fprintf(stderr, "rash: expected string after ‘<<<’.\n");
      return true;
    }

    // if (tokens[i].type ==)
  }

  return false;
}

int evaluate(const token_t *const tokens) {
  if (bad_syntax(tokens)) {
    return EXIT_FAILURE;
  }

  VECTOR(char *) argv;
  VECTOR_INIT(argv);

  int last_status = -1;

  execution_context ec = {NULL, -1, -1, -1, 0};

  for (size_t i = 0;; i++) {
    if (tokens[i].type == STRING) {
      VECTOR_PUSH(argv, (char *)tokens[i].data);

      continue;
    }

    if (tokens[i].type == STDIN_REDIR) {
      // this should've been taken care of with the call to bad syntax, but you
      // never know
      assert(tokens[i + 1].type == STRING);

      int fd = open((char *)tokens[i + 1].data, O_RDONLY);
      if (fd == -1) {
        fprintf(stderr, "rash: ");
        perror((char *)tokens[i + 1].data);

        goto error;
      }

      ec.stdin_fd = fd;

      i++;
      continue;
    }

    if (tokens[i].type == STDIN_REDIR_STRING) {
      assert(tokens[i + 1].type == STRING);

      int fds[2];
      if (pipe(fds) == -1) {
        perror("pipe");

        goto error;
      }

      size_t len = strlen((char *)tokens[i + 1].data);
      if (write(fds[1], tokens[i + 1].data, len) == -1) {
        perror("write");

        goto error;
      }

      if (close(fds[1]) == -1) {
        perror("close");

        goto error;
      }

      ec.stdin_fd = fds[0];

      i++;
      continue;
    }

    if (tokens[i].type == STDOUT_REDIR) {
      assert(tokens[i + 1].type == STRING);

      int fd = open(tokens[i + 1].data, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        fprintf(stderr, "rash: ");
        perror((char *)tokens[i + 1].data);

        goto error;
      }

      ec.stdout_fd = fd;

      i++;
      continue;
    }

    if (tokens[i].type == STDOUT_REDIR_APPEND) {
      assert(tokens[i + 1].type == STRING);

      int fd = open(tokens[i + 1].data, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        fprintf(stderr, "rash: ");
        perror((char *)tokens[i + 1].data);

        goto error;
      }

      ec.stdout_fd = fd;

      i++;
      continue;
    }

    if (tokens[i].type == STDERR_REDIR) {
      assert(tokens[i + 1].type == STRING);

      int fd = open(tokens[i + 1].data, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        fprintf(stderr, "rash: ");
        perror((char *)tokens[i + 1].data);

        goto error;
      }

      ec.stderr_fd = fd;

      i++;
      continue;
    }

    if (tokens[i].type == STDERR_REDIR_APPEND) {
      assert(tokens[i + 1].type == STRING);

      int fd = open(tokens[i + 1].data, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        fprintf(stderr, "rash: ");
        perror((char *)tokens[i + 1].data);

        goto error;
      }

      ec.stderr_fd = fd;

      i++;
      continue;
    }

    if (tokens[i].type == AMP) {
      VECTOR_PUSH(argv, NULL);
      ec.argv = argv.data;
      ec.flags = EC_BACKGROUND_JOB;
      last_status = execute(ec);
      ec = (execution_context){NULL, -1, -1, -1, 0};

      VECTOR_CLEAR(argv);
      break;
    }

    if (tokens[i].type == END) {
      VECTOR_PUSH(argv, NULL);
      ec.argv = argv.data;
      last_status = execute(ec);
      ec = (execution_context){NULL, -1, -1, -1, 0};

      VECTOR_CLEAR(argv);
      break;
    }
  }

  return last_status;

error:
  VECTOR_DESTROY(argv);

  return EXIT_FAILURE;
}
