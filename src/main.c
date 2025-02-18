#include "execute.h"
#include "lexer.h"
#include "line_reader.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <unistd.h>

bool should_exit = false;

volatile sig_atomic_t spawned_pid = 0;

static void sig_handler(int sig) {
  if (spawned_pid != 0) {
    kill((pid_t)spawned_pid, sig);
    write(STDOUT_FILENO, "\n", 1);
  }
  sigaction(SIGINT,
            &(struct sigaction){.sa_handler = sig_handler, .sa_flags = 0},
            NULL);
}

int main(int argc, char **argv) {
  FILE *fp = NULL;
  bool interactive = false;

  _Static_assert(sizeof(sig_atomic_t) == sizeof(pid_t),
                 "size of sig_atomic_t differs from pid_t. what the hell are "
                 "you compiling this on?");

  if (argc == 2) {
    fp = fopen(argv[1], "r");

    if (fp == NULL) {
      perror(argv[1]);
      return EXIT_FAILURE;
    }
  } else if (argc == 1) {
    interactive = true;
    fp = stdin;
  } else {
    fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
    return EXIT_FAILURE;
  }

  struct sigaction sa = {0};
  sa.sa_handler = sig_handler;
  sa.sa_flags = 0;

  // Set up SIGINT handler using sigaction
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction");
    fclose(fp);
    return EXIT_FAILURE;
  }

  char *line = NULL;
  size_t line_size = 0;
  int status = EXIT_SUCCESS;

  setenv("PS1", "$ ", 0);

  while (!should_exit) {
    if (interactive) {
      line = readline(line, getenv("PS1"));

      if (line == NULL) {
        break;
      }
    } else {
      ssize_t getline_status = getline(&line, &line_size, fp);

      if (getline_status == 0) {
        break;
      } else if (getline_status == -1) {
        if (!feof(fp)) {
          perror("getline");
        }

        break;
      }
    }

    char **tokens = get_tokens_from_line(line);

    if (tokens != NULL) {
      status = execute(tokens);

      free(tokens);
    } else {
      status = EXIT_FAILURE;
    }

    if (should_exit) {
      break;
    }

    // ? + = + 3 digit number (exit status is max of 255) + null terminator
    char status_env[3 + 1] = {0};

    sprintf(status_env, "%d", status);
    setenv("?", status_env, 1);
  }

  free(line);

  fclose(fp);

  return status;
}
