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
    (void) write(STDOUT_FILENO, "\n", 1);
  }
  sigaction(SIGINT,
            &(struct sigaction){.sa_handler = sig_handler, .sa_flags = 0},
            NULL);
}

_Static_assert(sizeof(sig_atomic_t) == sizeof(pid_t),
                 "size of sig_atomic_t differs from pid_t. what the hell are "
                 "you compiling this on?");

int main(int argc, char **argv) {
  FILE *file = NULL;
  bool interactive = false;

  if (argc == 2) {
    file = fopen(argv[1], "r");

    if (file == NULL) {
      perror(argv[1]);
      return EXIT_FAILURE;
    }
  } else if (argc == 1) {
    interactive = true;
    file = stdin;
  } else {
    fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
    return EXIT_FAILURE;
  }

  struct sigaction action = {0};
  action.sa_handler = sig_handler;
  action.sa_flags = 0;

  // Set up SIGINT handler using sigaction
  if (sigaction(SIGINT, &action, NULL) == -1) {
    perror("sigaction");
    fclose(file);
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
      ssize_t getline_status = getline(&line, &line_size, file);

      if (getline_status == 0) {
        break;
      }
      if (getline_status == -1) {
        if (!feof(file)) {
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

  fclose(file);

  return status;
}
