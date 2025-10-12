#include "evaluate.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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

int evaluate(const token_t *tokens) {
  if (bad_syntax(tokens)) {
    return EXIT_FAILURE;
  }

  VECTOR(char *) argv;
  VECTOR_INIT(argv);

  int last_status = -1;

  VECTOR(pid_t) wait_for_me;
  VECTOR_INIT(wait_for_me, 0);

  execution_context ec = {NULL, -1, -1, -1, 0};

  for (;; tokens++) {
    if (tokens->type == STRING) {
      VECTOR_PUSH(argv, (char *)tokens->data);

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

    if (tokens->type == LOGICAL_AND) {
      if (last_status == 0) {
        VECTOR_PUSH(argv, NULL);
        ec.argv = argv.data;
        last_status = execute(ec);
        ec = (execution_context){NULL, -1, -1, -1, 0};

        VECTOR_CLEAR(argv);
      } else {
        while ((tokens + 1)->type == STRING) {
          tokens++;
        }
      }

      continue;
    }

    if (tokens->type == LOGICAL_OR) {
      if (last_status != 0) {
        VECTOR_PUSH(argv, NULL);
        ec.argv = argv.data;
        last_status = execute(ec);
        ec = (execution_context){NULL, -1, -1, -1, 0};

        VECTOR_CLEAR(argv);
      } else {
        while ((tokens + 1)->type == STRING) {
          tokens++;
        }
      }

      continue;
    }

    if (tokens->type == AMP) {
      VECTOR_PUSH(argv, NULL);
      ec.argv = argv.data;
      ec.flags = EC_BACKGROUND_JOB;
      last_status = execute(ec);
      ec = (execution_context){NULL, -1, -1, -1, 0};

      VECTOR_CLEAR(argv);
      continue;
    }

    if (tokens->type == SEMI) {
      VECTOR_PUSH(argv, NULL);
      ec.argv = argv.data;
      last_status = execute(ec);
      ec = (execution_context){NULL, -1, -1, -1, 0};

      VECTOR_CLEAR(argv);
      continue;
    }

    if (tokens->type == END) {
      if (argv.length != 0) {
        VECTOR_PUSH(argv, NULL);
        ec.argv = argv.data;
        last_status = execute(ec);
      }
      break;
    }
  }

  for (size_t i = 0; i < wait_for_me.length; i++) {
    (void)waitpid(wait_for_me.data[i], NULL, 0);
  }

  return last_status;

error:
  VECTOR_DESTROY(argv);

  return EXIT_FAILURE;
}
