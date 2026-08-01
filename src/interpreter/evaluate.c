#include "evaluate.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "argv0.h"
#include "execute.h"
#include "glob.h"
#include "interpreter/token.h"
#include "lex.h"
#include "lib/buffer.h"
#include "lib/cstrlist.h"
#include "lib/error.h"
#include "lib/vector.h"
#include "shell_vars/shell_vars.h"

typedef struct {
  const TokenList *tokens;
  size_t current;
} EvalState;

static bool is_at_end(EvalState *s) {
  return s->current >= s->tokens->length;
}

static Token advance(EvalState *s) {
  if (is_at_end(s)) {
    return (Token){.kind = TK_NONE};
  }

  return s->tokens->data[s->current++];
}

static Token peek(EvalState *s) {
  if (is_at_end(s)) {
    return (Token){.kind = TK_NONE};
  }

  return s->tokens->data[s->current];
}

static Token peek_prev(EvalState *s) {
  if (s->current == 0) {
    return (Token){.kind = TK_NONE};
  }

  return s->tokens->data[s->current - 1];
}

static bool check(EvalState *s, TokenKind kind) {
  if (is_at_end(s)) {
    return false;
  }
  return peek(s).kind == kind;
}

static bool match(EvalState *s, TokenKind kind) {
  if (check(s, kind)) {
    advance(s);
    return true;
  }

  return false;
}

static bool match_argument(EvalState *s) {
  if (peek(s).kind == TK_ARG_END) {
    advance(s);
    return true;
  }

  if (!IS_ARGUMENT_TOKEN(peek(s).kind)) {
    return false;
  }

  while (IS_ARGUMENT_TOKEN(peek(s).kind)) {
    advance(s);
  }

  if (!match(s, TK_ARG_END)) {
    // lexer should guarentee this
    assert(false);
  }
  
  return true;
}

static bool bad_syntax(const TokenList *tokens) {
  EvalState s = {
    .tokens = tokens,
    .current = 0
  };

  if (check(&s, TK_ARG_END)) {
    error("rash: empty string is not a valid command.\n");
    return true;
  }

  if (!IS_ARGUMENT_TOKEN(peek(&s).kind)) {
    error("rash: invalid first token.\n");
    return true;
  }

  int stdout_count = 0;
  int stderr_count = 0;
  int stdin_count = 0;

  while (!is_at_end(&s)) {
    if (match_argument(&s)) {
      continue;
    }

    if (match(&s, TK_STDIN_REDIR)) {
      if (!match_argument(&s)) {
        error_f("rash: expected filename after ‘<’.\n");
        return true;
      }

      stdin_count++;
    }

    if (match(&s, TK_STDIN_REDIR_STRING)) {
      if (!match_argument(&s)) {
        error_f("rash: expected string after ‘<<<’.\n");
        return true;
      }

      stdin_count++;
    }

    if (match(&s, TK_STDOUT_REDIR)) {
     if (!match_argument(&s)) {
        error_f("rash: expected filename after ‘>’.\n");
        return true;
      }

      stdout_count++;
    }

    if (match(&s, TK_STDOUT_REDIR_APPEND)) {
      if (!match_argument(&s)) {
        error_f("rash: expected filename after ‘>>’.\n");
        return true;
      }

      stdout_count++;
    }

    if (match(&s, TK_STDERR_REDIR)) {
      if (!match_argument(&s)) {
        error_f("rash: expected filename after ‘2>’.\n");
        return true;
      }

      stderr_count++;
    }

    if (match(&s, TK_STDERR_REDIR_APPEND)) {
      if (!match_argument(&s)) {
        error_f("rash: expected filename after ‘2>>’.\n");
        return true;
      }

      stderr_count++;
    }

    if (match(&s, TK_STDOUT_ERR_REDIR)) {
      if (!match_argument(&s)) {
        error_f("rash: expected filename after ‘&>’.\n");
        return true;
      }

      stdout_count++;
      stderr_count++;
    }

    if (match(&s, TK_STDOUT_ERR_REDIR_APPEND)) {
      if (!match_argument(&s)) {
        error_f("rash: expected filename after ‘&>>’.\n");
        return true;
      }

      stdout_count++;
      stderr_count++;
    }

    bool pipe_flag = false;
    if (match(&s, TK_PIPE)) {
      pipe_flag = true;

      if (stdout_count > 0) {
        error("rash: cannot redirect stdout and pipe into another command.\n");
        return true;
      }
      
      if (!match_argument(&s)) {
        error_f("rash: expected command after ‘|’.\n");
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

    // set redirection conditions for next command after the pipe
    if (pipe_flag) {
      stdout_count = 0;
      stdin_count = 1;
      stderr_count = 0;
    }

    if (match(&s, TK_LOGICAL_OR)) {
      if (!match_argument(&s)) {
        error_f("rash: expected command after ‘||’.\n");
        return true;
      }

      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }

    if (match(&s, TK_LOGICAL_AND)) {
      if (!match_argument(&s)) {
        error_f("rash: expected command after ‘&&’.\n");
        return true;
      }

      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }

    if (match(&s, TK_SEMI)) {
      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }

    if (match(&s, TK_AMP)) {
      stdout_count = 0;
      stdin_count = 0;
      stderr_count = 0;
    }
  }

  return false;
}

static void set_exit_code_var(int code) {
  ShellVar *var = var_create_number((double)(code & 0xff));

  var_set("LAST_STATUS", var);

  var_release(var);
}

static CStrList evaluate_arg(EvalState *s) {
  Buffer buffer = buffer_create(16);
  bool needs_globbing = false;

  while (1) {
    if (check(s, TK_ARG_STRING)) {
      Token token = advance(s);

      buffer_append(&buffer, &token.buffer);

      continue;
    }

    if (check(s, TK_ARG_ENV)) {
      Token token = advance(s);

      Buffer env = buffer_clone(&token.buffer);
      const char *value = getenv(buffer_cstr(&env));
      buffer_destroy(&env);

      if (value == NULL) {
        error_f(
          "rash: environment variable ‘%.*s’ does not exist.\n",
          (int)token.buffer.length,
          token.buffer.char_ptr
        );
        goto error;
      }

      buffer_append(&buffer, value);

      continue;
    }

    if (check(s, TK_ARG_TILDE)) {
      Token token = advance(s);

      if (token.buffer.length == 0) {
        char *home = getenv("HOME");
        if (home != NULL) {
          buffer_append(&buffer, home);
          continue;
        }
        error_f("cannot expand ‘~’, HOME is not set.\n");
        goto error;
      }

      Buffer username = buffer_clone(&token.buffer);
      struct passwd *pw = getpwnam(buffer_cstr(&username));
      buffer_destroy(&username);

      if (pw == NULL || pw->pw_dir == NULL) {
        error_f(
          "rash: cannot access user ‘%.*s’.\n",
          (int)token.buffer.length,
          token.buffer.char_ptr
        );
        goto error;
      }

      buffer_append(&buffer, pw->pw_dir);
      continue;
    }

    if (check(s, TK_ARG_SHELL_EXPR)) {
      Token token = advance(s);

      Buffer expr = buffer_clone(&token.buffer);
      char *value = var_eval_to_string(buffer_cstr(&expr));
      buffer_destroy(&expr);

      if (value == NULL) {
        // var_eval_to_string prints an error message
        goto error;
      }

      buffer_append(&buffer, value);
      free(value);

      continue;
    }

    if (check(s, TK_ARG_SUBSHELL)) {
      Token token = advance(s);

      Buffer cmd = buffer_clone(&token.buffer);

      char *argv[] = {argv0, "-c", buffer_cstr(&cmd), NULL};

      int null_fd = open("/dev/null", O_RDWR);

      rash_assert(null_fd != -1, "cannot open /dev/null");

      int null_fd2 = dup(null_fd);

      rash_assert(null_fd2 != -1, "dup failed");

      int fds[2];

      if (pipe(fds) == -1) {
        rash_assert(0, "pipe failed");
      }

      ExecutionContext ec = {
        .argv = argv,
        .flags = EC_NO_WAIT,
        .stderr_fd = null_fd,
        .stdin_fd = null_fd2,
        .stdout_fd = fds[1]
      };

      pid_t pid = execute(ec);

      if (pid == -1) {
        goto error;
      }

      uint8_t read_bytes[512];
      ssize_t nread;

      do {
        nread = read(fds[0], read_bytes, 512);

        rash_assert(nread != -1, "read failed");

        for (ssize_t i = 0; i < nread; i++) {
          if (!iscntrl((int)read_bytes[i])) {
            buffer_append(&buffer, read_bytes[i]);
          }
        }
      } while (nread > 0);

      buffer_destroy(&cmd);
      wait_process(pid);
      continue;
    }

    if (match(s, TK_ARG_WILDCARD)) {
      // this is a really dumb solution to this problem, but the line reader
      // assures that '\033' never be in the string, so it's not bad unless i
      // forget to strip out '\033' when i implement shell scripts. also if
      // futures globs besides the wildcard are added, this will need to be
      // reworked
      buffer_append(&buffer, '\033');
      needs_globbing = true;
      continue;
    }

    if (match(s, TK_ARG_END)) {
      break;
    }
  }

  CStrList list = {0};

  if (needs_globbing) {
    int args_added = glob(&list, buffer_cstr(&buffer));
    if (args_added == 0) {
      for (size_t i = 0; buffer.length; i++) {
        if (buffer.u8_ptr[i] == '\033') {
          buffer.u8_ptr[i] = '*';
        }
      }

      error_f(
        "rash: nothing matched glob pattern ‘%.*s’.\n",
        (int)buffer.length,
        buffer.char_ptr
      );
      
      goto error;
    }

    if (args_added == -1) {
      goto error;
    }

    return list;
  }

  VECTOR_PUSH(list, buffer_cstr(&buffer));
  return list;

error:
  buffer_destroy(&buffer);
  return (CStrList){0};
}

int evaluate(const TokenList *tokens) {
  if (bad_syntax(tokens)) {
    return EXIT_FAILURE;
  }

  CStrList argv;
  VECTOR_INIT(argv);

  int last_status = -1;

  VECTOR(pid_t) wait_for_me = {0, 0, 0};
  // this doesn't compile on gcc :(
  // VECTOR_INIT(wait_for_me, 0);

  ExecutionContext ec = {
    .argv = NULL,
    .stdout_fd = -1,
    .stdin_fd = -1,
    .stderr_fd = -1,
    .flags = 0
  };

  EvalState s = {
    .tokens = tokens,
    .current = 0
  };

  while (!is_at_end(&s)) {
    if (IS_ARGUMENT_TOKEN(peek(&s).kind)) {
      CStrList arguments = evaluate_arg(&s);

      if (arguments.length) {
        VECTOR_DESTROY(arguments);
        goto error;
      }

      for (size_t i = 0; i < arguments.length; i++) {
        VECTOR_PUSH(argv, arguments.data[i]);
      }
      continue;
    }

    if (match(&s, TK_STDIN_REDIR)) {
      CStrList arguments = evaluate_arg(&s);

      if (arguments.length == 0) {
        cstr_list_destroy(&arguments);
        goto error;
      }

      if (arguments.length != 1) {
        error_f("rash: string following `<` expands to multiple arguments but is used as a filename where one argument is required.\n");
        cstr_list_destroy(&arguments);
        goto error;
      }

      int fd = open(arguments.data[0], O_RDONLY);
      if (fd == -1) {
        error_f("rash: %s: %s\n", arguments.data[0], strerror(errno));
        cstr_list_destroy(&arguments);
        goto error;
      }

      cstr_list_destroy(&arguments);

      ec.stdin_fd = fd;

      continue;
    }

    if (match(&s, TK_STDIN_REDIR_STRING)) {
      CStrList arguments = evaluate_arg(&s);

      if (arguments.length == 0) {
        cstr_list_destroy(&arguments);
        goto error;
      }

      if (arguments.length != 1) {
        error_f("rash: string following `<<<` expands to multiple arguments but is used as a filename where one argument is required.\n");
        cstr_list_destroy(&arguments);
        goto error;
      }

      int fds[2];
      if (pipe(fds) == -1) {
        rash_assert(0, "pipe failed");
      }

      char *str = arguments.data[0];
      
      size_t len = strlen(str);
      ssize_t written = write(fds[1], str, len);

      if (written != len) {
        error_f("wrote %zd bytes\n", written);
        rash_assert(0, "write failed");
      }

      cstr_list_destroy(&arguments);

      ec.stdin_fd = fds[0];

      continue;
    }

    if (match(&s, TK_STDOUT_REDIR)) {
      CStrList arguments = evaluate_arg(&s);

      if (arguments.length == 0) {
        cstr_list_destroy(&arguments);
        goto error;
      }

      if (arguments.length != 1) {
        error_f("rash: string following `>` expands to multiple arguments but is used as a filename where one argument is required.\n");
        cstr_list_destroy(&arguments);
        goto error;
      }

      char *filename = arguments.data[0];
      int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        error_f("rash: %s: %s\n", arguments.data[0], strerror(errno));
        cstr_list_destroy(&arguments);
        goto error;
      }

      cstr_list_destroy(&arguments);

      ec.stdout_fd = fd;

      continue;
    }

    if (match(&s, TK_STDOUT_REDIR_APPEND)) {
      CStrList arguments = evaluate_arg(&s);

      if (arguments.length == 0) {
        cstr_list_destroy(&arguments);
        goto error;
      }

      if (arguments.length != 1) {
        error_f("rash: string following `>>` expands to multiple arguments but is used as a filename where one argument is required.\n");
        cstr_list_destroy(&arguments);
        goto error;
      }

      char *filename = arguments.data[0];
      int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        error_f("rash: %s: %s\n", arguments.data[0], strerror(errno));
        cstr_list_destroy(&arguments);
        goto error;
      }

      cstr_list_destroy(&arguments);

      ec.stdout_fd = fd;

      continue;
    }

    if (match(&s, TK_STDERR_REDIR)) {
      CStrList arguments = evaluate_arg(&s);

      if (arguments.length == 0) {
        cstr_list_destroy(&arguments);
        goto error;
      }

      if (arguments.length != 1) {
        error_f("rash: string following `2>` expands to multiple arguments but is used as a filename where one argument is required.\n");
        cstr_list_destroy(&arguments);
        goto error;
      }

      char *filename = arguments.data[0];
      int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        error_f("rash: %s: %s\n", arguments.data[0], strerror(errno));
        cstr_list_destroy(&arguments);
        goto error;
      }

      cstr_list_destroy(&arguments);

      ec.stderr_fd = fd;

      continue;
    }

    if (match(&s, TK_STDERR_REDIR_APPEND)) {
      CStrList arguments = evaluate_arg(&s);

      if (arguments.length == 0) {
        cstr_list_destroy(&arguments);
        goto error;
      }

      if (arguments.length != 1) {
        error_f("rash: string following `2>>` expands to multiple arguments but is used as a filename where one argument is required.\n");
        cstr_list_destroy(&arguments);
        goto error;
      }

      char *filename = arguments.data[0];
      int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        error_f("rash: %s: %s\n", arguments.data[0], strerror(errno));
        cstr_list_destroy(&arguments);
        goto error;
      }

      cstr_list_destroy(&arguments);

      ec.stderr_fd = fd;

      continue;
    }

    if (match(&s, TK_STDOUT_ERR_REDIR)) {
      CStrList arguments = evaluate_arg(&s);

      if (arguments.length == 0) {
        cstr_list_destroy(&arguments);
        goto error;
      }

      if (arguments.length != 1) {
        error_f("rash: string following `&>` expands to multiple arguments but is used as a filename where one argument is required.\n");
        cstr_list_destroy(&arguments);
        goto error;
      }

      char *filename = arguments.data[0];
      int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        error_f("rash: %s: %s\n", arguments.data[0], strerror(errno));
        cstr_list_destroy(&arguments);
        goto error;
      }

      int fd2 = dup(fd);

      if (fd2 == -1) {
        rash_assert(0, "dup failed");
      }

      cstr_list_destroy(&arguments);

      ec.stdout_fd = fd;
      ec.stderr_fd = fd2;

      continue;
    }

    if (match(&s, TK_STDOUT_ERR_REDIR_APPEND)) {
      CStrList arguments = evaluate_arg(&s);

      if (arguments.length == 0) {
        cstr_list_destroy(&arguments);
        goto error;
      }

      if (arguments.length != 1) {
        error_f("rash: string following `&>>` expands to multiple arguments but is used as a filename where one argument is required.\n");
        cstr_list_destroy(&arguments);
        goto error;
      }

      char *filename = arguments.data[0];
      int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        error_f("rash: %s: %s\n", arguments.data[0], strerror(errno));
        cstr_list_destroy(&arguments);
        goto error;
      }

      int fd2 = dup(fd);

      if (fd2 == -1) {
        rash_assert(0, "dup failed");
      }

      cstr_list_destroy(&arguments);

      ec.stdout_fd = fd;
      ec.stderr_fd = fd2;

      continue;
    }

    if (match(&s, TK_PIPE)) {
      VECTOR_PUSH(argv, NULL);

      int fds[2];
      if (pipe(fds) == -1) {
        rash_assert(0, "pipe failed");
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

      VECTOR_CLEAR(argv);
      continue;
    }

    if (match(&s, TK_AMP)) {
      ec.flags = EC_BACKGROUND_JOB;
      continue;
    }

    if (argv.length > 0) {
      VECTOR_PUSH(argv, NULL);
      ec.argv = argv.data;
      last_status = execute(ec);
      set_exit_code_var(last_status);
      ec = (ExecutionContext){NULL, -1, -1, -1, 0};

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

    if (match(&s, TK_LOGICAL_AND)) {
      if (last_status != 0) {
        match_argument(&s);
      }

      continue;
    }

    if (tokens->type == TK_LOGICAL_OR) {
      if (last_status == 0) {
        while ((tokens + 1)->type == TK_END_ARG) {
          tokens++;
        }
      }

      continue;
    }

    if (tokens->type == TK_SEMI) {
      continue;
    }

    if (tokens->type == TK_END) {
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
    rash_assert(id != -1);
  }
  VECTOR_DESTROY(wait_for_me);
  for (size_t i = 0; i < argv.length; i++) {
    free(argv.data[i]);
  }
  VECTOR_DESTROY(argv);

  return EXIT_FAILURE;
}
