#include "interpreter/evaluate.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "global.h"
#include "interpreter/execute.h"
#include "interpreter/glob.h"
#include "interpreter/token.h"
#include "lib/buffer.h"
#include "lib/cstrlist.h"
#include "lib/error.h"
#include "lib/sys.h"
#include "lib/vector.h"
#include "shell_vars/eval.h"
#include "shell_vars/functions.h"
#include "shell_vars/shell_vars.h"

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1

static bool is_at_end(EvalState *s) {
  return s->current >= s->tokens->length;
}

static const Token NONE_TOKEN = { .kind = TK_NONE };

static const Token *advance(EvalState *s) {
  if (is_at_end(s)) {
    return &NONE_TOKEN;
  }

  return s->tokens->data + (s->current++);
}

static const Token *peek(EvalState *s) {
  if (is_at_end(s)) {
    return &NONE_TOKEN;
  }

  return s->tokens->data + s->current;
}

static bool check(EvalState *s, TokenKind kind) {
  if (is_at_end(s)) {
    return false;
  }
  return peek(s)->kind == kind;
}

static bool match(EvalState *s, TokenKind kind) {
  if (check(s, kind)) {
    advance(s);
    return true;
  }

  return false;
}

static void set_exit_code_var(int code) {
  ShellVar *var = var_create_number((double)((unsigned)code & 0xffU));

  var_state_var_set(&instance.var_state, "LAST_STATUS", var);

  var_release(var);
}

static int insert_env(Buffer *buffer, const Buffer *env_name) {
  char *env = buffer_clone_to_cstr(env_name);
  const char *value = getenv(env);
  free(env);

  if (value == NULL) {
    error_f(
        "rash: environment variable ‘%.*s’ does not exist.\n",
        (int)buffer->length,
        buffer->char_ptr
    );
    return -1;
  }

  buffer_append(buffer, value);
  return 0;
}

static int insert_shell_expr(Buffer *buffer, EvalState *s) {
  ShellVar *var = evaluate_tokens(s);
  if (var == NULL) {
    return -1;
  }

  if (!match(s, TK_SE_END)) {
    error("shell expression: expected end of input.\n");
    var_release(var);
    return -1;
  }

  Buffer var_str = var_to_string(var);
  var_release(var);

  buffer_append(buffer, &var_str);
  buffer_destroy(&var_str);

  return 0;
}

static int insert_tilde(Buffer *buffer, const Buffer *username) {
  if (username->length == 0) {
    char *home = getenv("HOME");
    if (home != NULL) {
      buffer_append(buffer, home);
      return 0;
    }
    error_f("cannot expand ‘~’, HOME is not set.\n");
    return -1;
  }

  char *user_cstr = buffer_clone_to_cstr(username);
  struct passwd *pw = getpwnam(user_cstr);
  free(user_cstr);

  if (pw == NULL || pw->pw_dir == NULL) {
    error_f(
        "rash: cannot access user ‘%.*s’.\n",
        (int)username->length,
        username->char_ptr
    );
    return -1;
  }

  buffer_append(buffer, pw->pw_dir);
  return 0;
}

static int insert_subshell(Buffer *buffer, const Buffer *command) {
  // open /dev/null for stdin on the subshell process
  int null_fd = open("/dev/null", O_RDWR);
  rash_assert(null_fd != -1, "cannot open /dev/null");

  // need another /dev/null for stderr
  int null_fd2 = dup(null_fd);
  rash_assert(null_fd2 != -1, "dup failed");

  int fds[2];
  if (pipe(fds) == -1) {
    rash_assert(0, "pipe failed");
  }

  // launch rash in one-shot mode with the command
  char *cmd_cstr = buffer_clone_to_cstr(command);
  char *argv[] = {instance.argv0, "-c", cmd_cstr, NULL};
  ExecutionContext ec = {
      .argv = argv,
      .flags = EC_NO_WAIT,
      .stderr_fd = null_fd,
      .stdin_fd = null_fd2,
      .stdout_fd = fds[1]
  };

  pid_t pid = execute(ec);

  free(cmd_cstr);

  if (pid == -1) {
    return -1;
  }

  uint8_t read_bytes[512];
  ssize_t nread;

  do {
    nread = read(fds[0], read_bytes, 512);

    rash_assert(nread != -1, "read failed");

    for (ssize_t i = 0; i < nread; i++) {
      if (!iscntrl((int)read_bytes[i])) {
        buffer_append(buffer, read_bytes[i]);
      }
    }
  } while (nread > 0);

  wait_process(pid);
  return 0;
}

static char *internal_evaluate_arg(EvalState *s, bool *needs_globbing) {
  Buffer buffer = buffer_create(16);

  while (1) {
    if (check(s, TK_ARG_STRING)) {
      const Token *token = advance(s);
      buffer_append(&buffer, &token->buffer);
      continue;
    }

    if (check(s, TK_ARG_ENV)) {
      const Token *token = advance(s);
      if (insert_env(&buffer, &token->buffer)) {
        goto error;
      }
      continue;
    }

    if (check(s, TK_ARG_TILDE)) {
      const Token *token = advance(s);
      if (insert_tilde(&buffer, &token->buffer)) {
        goto error;
      }
      continue;
    }

    if (match(s, TK_SE_BEGIN)) {
      if (insert_shell_expr(&buffer, s)) {
        goto error;
      }
      continue;
    }

    if (check(s, TK_ARG_SUBSHELL)) {
      const Token *token = advance(s);
      if (insert_subshell(&buffer, &token->buffer)) {
        goto error;
      }
      continue;
    }

    if (match(s, TK_ARG_WILDCARD)) {
      // this is a really dumb solution to this problem, but the line reader
      // assures that '\033' never be in the string, so it's not bad unless i
      // forget to strip out '\033' when i implement shell scripts. also if
      // futures globs besides the wildcard are added, this will need to be
      // reworked
      buffer_append(&buffer, '\033');
      *needs_globbing = true;
      continue;
    }

    if (match(s, TK_ARG_END)) {
      break;
    }
  }

  return buffer_cstr(&buffer);

error:
  buffer_destroy(&buffer);
  return NULL;
}

// used to extract a single argument, will fail is a glob expression returns
// multiple files
static char *evaluate_arg(EvalState *s) {
  bool needs_globbing = false;
  char *arg = internal_evaluate_arg(s, &needs_globbing);

  if (arg == NULL) {
    return NULL;
  }

  if (!needs_globbing) {
    return arg;
  }

  CStrList list = {0};
  int args_added = glob(&list, arg);
  for (size_t i = 0; arg[i] != '\0'; i++) {
    if (arg[i] == '\033') {
      arg[i] = '*';
    }
  }

  // globbing failed
  if (args_added == -1) {
    goto error;
  }

  if (args_added == 0) {
    error_f("rash: nothing matched glob pattern ‘%s’.\n", arg);
    goto error;
  }

  if (args_added != 1) {
    error_f(
        "rash: glob expression `%s` expanded to multiple files when only one "
        "file was expected.\n",
        arg
    );
    goto error;
  }

  char *ptr = list.data[0];
  free(arg);
  free(list.data);
  return ptr;

error:
  free(arg);
  return NULL;
}

// used to extract multipe arguments, if a glob expression returns multiple
// files they will all be in the returned list
static CStrList evaluate_args(EvalState *s) {
  CStrList list = {0};

  bool needs_globbing = false;
  char *arg = internal_evaluate_arg(s, &needs_globbing);

  if (arg == NULL) {
    return list;
  }

  if (!needs_globbing) {
    VECTOR_PUSH(list, arg);
    return list;
  }

  int args_added = glob(&list, arg);
  for (size_t i = 0; arg[i] != '\0'; i++) {
    if (arg[i] == '\033') {
      arg[i] = '*';
    }
  }

  // globbing failed
  if (args_added == -1) {
    goto error;
  }

  if (args_added == 0) {
    error_f("rash: nothing matched glob pattern ‘%s’.\n", arg);
    goto error;
  }

  free(arg);
  return list;

error:
  free(arg);
  VECTOR_DESTROY(list);
  return (CStrList){0};
}

typedef enum {
  STDOUT_DEFAULT,
  STDOUT_FILE_APPEND,
  STDOUT_FILE_TRUNCATE,
  STDOUT_PIPE
} StdoutKind;

typedef enum {
  STDERR_DEFAULT,
  STDERR_FILE_APPEND,
  STDERR_FILE_TRUNCATE
} StderrKind;

typedef enum {
  STDIN_DEFAULT,
  STDIN_FILE,
  STDIN_STRING
} StdinKind;

typedef struct Command {
  char *const *argv;

  bool background;

  struct {
    StdoutKind kind;
    union {
      char *filename;
      struct Command *pipe_to;
    };
  } stdout_;

  struct {
    StderrKind kind;
    char *filename;
  } stderr_;

  struct {
    StdinKind kind;
    union {
      char *filename;
      char *string;
    };
  } stdin_;
} Command;

static void command_destroy(Command *cmd) { // NOLINT(misc-no-recursion)
  if (cmd->argv != NULL) {
    for (size_t i = 0; cmd->argv[i] != NULL; i++) {
      free(cmd->argv[i]);
    }
  }

  switch (cmd->stdout_.kind) {
    case STDOUT_DEFAULT:
      break;
    case STDOUT_FILE_APPEND:
    case STDOUT_FILE_TRUNCATE:
      free(cmd->stdout_.filename);
      break;
    case STDOUT_PIPE:
      command_destroy(cmd->stdout_.pipe_to);
      break;
  }

  switch (cmd->stderr_.kind) {
    case STDERR_DEFAULT:
      break;
    case STDERR_FILE_APPEND:
    case STDERR_FILE_TRUNCATE:
      free(cmd->stderr_.filename);
      break;
  }

  switch (cmd->stdin_.kind) {
    case STDIN_DEFAULT:
      break;
    case STDIN_FILE:
      free(cmd->stdin_.filename);
      break;
    case STDIN_STRING:
      free(cmd->stdin_.string);
      break;
  }

  free(cmd);
}

static char *read_string_after_token(EvalState *s, TokenKind kind) {
  if (!match(s, TK_ARG_BEGIN)) {
    error_f("rash: expected string after %s.\n", get_token_kind_name(kind));
    return NULL;
  }
  return evaluate_arg(s);
}

static Command *parse_command(EvalState *s) { // NOLINT(misc-no-recursion)
  CStrList argv = {0};

  Command *command = malloc(sizeof(Command));
  command->argv = NULL;
  command->background = false;
  command->stderr_.kind = STDERR_DEFAULT;
  command->stdin_.kind = STDIN_DEFAULT;
  command->stdout_.kind = STDOUT_DEFAULT;

  while (1) {
    if (match(s, TK_ARG_BEGIN)) {
      CStrList args = evaluate_args(s);
      if (args.length == 0) {
        VECTOR_DESTROY(args);
        goto error;
      }
      for (size_t i = 0; i < args.length; i++) {
        VECTOR_PUSH(argv, args.data[i]);
      }
      VECTOR_DESTROY(args);
      continue;
    }

    if (match(s, TK_AMP)) {
      if (command->background) {
        error("rash: duplicate `&` on command.\n");
        goto error;
      }
      command->background = true;
    }

    if (match(s, TK_STDIN_REDIR)) {
      char *filename = read_string_after_token(s, TK_STDIN_REDIR);

      if (filename == NULL) {
        goto error;
      }

      if (command->stdin_.kind != STDIN_DEFAULT) {
        error("rash: cannot redirect stdin more than once.\n");
        goto error;
      }
      
      command->stdin_.kind = STDIN_FILE;
      command->stdin_.filename = filename;
      continue;
    }

    if (match(s, TK_STDIN_REDIR_STRING)) {
      char *filename = read_string_after_token(s, TK_STDIN_REDIR_STRING);

      if (filename == NULL) {
        goto error;
      }

      if (command->stdin_.kind != STDIN_DEFAULT) {
        error("rash: cannot redirect stdin more than once.\n");
        goto error;
      }

      command->stdin_.kind = STDIN_STRING;
      command->stdin_.string = filename;
      continue;
    }

    if (match(s, TK_STDOUT_REDIR)) {
      char *filename = read_string_after_token(s, TK_STDOUT_REDIR);

      if (filename == NULL) {
        goto error;
      }

      if (command->stdout_.kind != STDOUT_DEFAULT) {
        error("rash: cannot redirect stdout more than once.\n");
        goto error;
      }

      command->stdout_.kind = STDOUT_FILE_TRUNCATE;
      command->stdout_.filename = filename;
      continue;
    }

    if (match(s, TK_STDOUT_REDIR_APPEND)) {
      char *filename = read_string_after_token(s, TK_STDOUT_REDIR_APPEND);

      if (filename == NULL) {
        goto error;
      }

      if (command->stdout_.kind != STDOUT_DEFAULT) {
        error("rash: cannot redirect stdout more than once.\n");
        goto error;
      }

      command->stdout_.kind = STDOUT_FILE_APPEND;
      command->stdout_.filename = filename;
      continue;
    }

    if (match(s, TK_STDERR_REDIR)) {
      char *filename = read_string_after_token(s, TK_STDERR_REDIR);

      if (filename == NULL) {
        goto error;
      }

      if (command->stderr_.kind != STDERR_DEFAULT) {
        error("rash: cannot redirect stdout more than once.\n");
        goto error;
      }
      command->stderr_.kind = STDERR_FILE_TRUNCATE;
      command->stderr_.filename = filename;
      continue;
    }

    if (match(s, TK_STDERR_REDIR_APPEND)) {
      char *filename = read_string_after_token(s, TK_STDERR_REDIR);

      if (filename == NULL) {
        goto error;
      }

      if (command->stderr_.kind != STDERR_DEFAULT) {
        error("rash: cannot redirect stderr more than once.\n");
        goto error;
      }
      command->stderr_.kind = STDERR_FILE_APPEND;
      command->stderr_.filename = filename;
      continue;
    }

    if (match(s, TK_STDOUT_ERR_REDIR)) {
      char *filename = read_string_after_token(s, TK_STDOUT_ERR_REDIR);

      if (filename == NULL) {
        goto error;
      }

      if (command->stdout_.kind != STDOUT_DEFAULT) {
        error("rash: cannot redirect stdout more than once.\n");
        goto error;
      }
      if (command->stderr_.kind != STDERR_DEFAULT) {
        error("rash: cannot redirect stdout more than once.\n");
        goto error;
      }
      command->stderr_.kind = STDERR_FILE_TRUNCATE;
      command->stderr_.filename = filename;
      command->stdout_.kind = STDOUT_FILE_TRUNCATE;
      command->stdout_.filename = strdup(filename);
      continue;
    }
    if (match(s, TK_STDOUT_ERR_REDIR_APPEND)) {
      char *filename = read_string_after_token(s, TK_STDOUT_ERR_REDIR_APPEND);

      if (filename == NULL) {
        goto error;
      }

      if (command->stdout_.kind != STDOUT_DEFAULT) {
        error("rash: cannot redirect stdout more than once.\n");
        goto error;
      }
      if (command->stderr_.kind != STDERR_DEFAULT) {
        error("rash: cannot redirect stdout more than once.\n");
        goto error;
      }
      command->stderr_.kind = STDERR_FILE_APPEND;
      command->stderr_.filename = filename;
      command->stdout_.kind = STDOUT_FILE_APPEND;
      command->stdout_.filename = strdup(filename);
      continue;
    }

    if (match(s, TK_PIPE)) {
      if (command->stdout_.kind != STDOUT_DEFAULT) {
        error("rash: cannot redirect stdout more than once.\n");
        goto error;
      }

      command->stdout_.pipe_to = parse_command(s);
      if (command->stdout_.pipe_to == NULL) {
        goto error;
      }
      command->stdout_.kind = STDOUT_PIPE;
    }
    break;
  }

  VECTOR_PUSH(argv, NULL);
  command->argv = argv.data;

  return command;

error:
  VECTOR_DESTROY(argv);
  command_destroy(command);
  return NULL;
}

/// returns the fd of the stdin to the next command in the pipeline, or 0 if no
/// further commands exist, -1 is returned on error
static int command_to_execution_context(
  ExecutionContext *out,
  const Command *command,
  int stdin_fd
) {
  int stdout_fd = -1;
  int stderr_fd = -1;

  // the fd of stdin to the next command in the pipeline
  int next_fd = 0;

  switch (command->stdin_.kind) {
    case STDIN_DEFAULT:
      break;
    case STDIN_FILE: {
      int fd = open(command->stdin_.filename, O_RDONLY);
      if (fd == -1) {
        perror("open");
        goto error;
      }

      stdin_fd = fd;
      break;
    }
    case STDIN_STRING: {
      int fds[2];
      if (pipe(fds) == -1) {
        perror("pipe");
        goto error;
      }

      // TODO: make this write all data
      write(fds[PIPE_WRITE_END], command->stdin_.string, strlen(command->stdin_.string));
      close_assert(fds[PIPE_WRITE_END]);

      stdin_fd = fds[PIPE_READ_END];
      break;
    }
  }

  switch (command->stdout_.kind) {
    case STDOUT_DEFAULT:
      break;
    case STDOUT_FILE_TRUNCATE: {
      stdout_fd = open(command->stdout_.filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
      if (stdout_fd == -1) {
        perror("open");
        goto error;
      }
      break;
    }
    case STDOUT_FILE_APPEND: {
      stdout_fd = open(command->stdout_.filename, O_WRONLY | O_CREAT | O_APPEND, 0664);
      if (stdout_fd == -1) {
        perror("open");
        goto error;
      }
      break;
    }
    case STDOUT_PIPE: {
      int fds[2] = {0};
      if (pipe(fds) == -1) {
        perror("pipe");
        goto error;
      }

      stdout_fd = fds[PIPE_WRITE_END];
      next_fd = fds[PIPE_READ_END];
      break;
    }
  }

  switch (command->stderr_.kind) {
    case STDERR_DEFAULT:
      break;
    case STDERR_FILE_TRUNCATE: {
      stderr_fd = open(command->stderr_.filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
      if (stderr_fd == -1) {
        perror("open");
        goto error;
      }
      break;
    }
    case STDERR_FILE_APPEND: {
      stderr_fd = open(command->stderr_.filename, O_WRONLY | O_CREAT | O_APPEND, 0664);
      if (stderr_fd == -1) {
        perror("open");
        goto error;
      }
      break;
    }
  }

  out->argv = command->argv;
  out->stderr_fd = stderr_fd;
  out->stdout_fd = stdout_fd;
  out->stdin_fd = stdin_fd;
  out->flags = 0;
  if (command->background) {
    out->flags |= EC_BACKGROUND_JOB;
  }

  return next_fd;

error:
  if (stderr_fd != -1) {
    close_assert(stderr_fd);
  }
  if (stdin_fd != -1) {
    close_assert(stdin_fd);
  }
  if (stdout_fd != -1) {
    close_assert(stdout_fd);
  }
  return -1;
}

static int dispatch_external_command(Command *command) {
  VECTOR(ExecutionContext) list = {0};

  int next_fd = -1;
  while (1) {
    ExecutionContext ec;
    ec.stdin_fd = next_fd;
    next_fd = command_to_execution_context(&ec, command, next_fd);

    if (next_fd == -1) {
      goto error;
    }

    VECTOR_PUSH(list, ec);

    if (next_fd == 0) {
      break;
    }

    assert(command->stdout_.kind == STDOUT_PIPE);
    command = command->stdout_.pipe_to;

    // this state should be invalid
    assert(command != NULL);
  }

  rash_assert(list.length != 0, "command list length should never be zero.");

  if (list.length == 1) {
    int status = execute(list.data[list.length - 1]);
    VECTOR_DESTROY(list);
    return status;
  }

  size_t background_cmd_count = list.length - 1;

  pid_t *pids = malloc(sizeof(pid_t) * background_cmd_count);

  for (size_t i = 0; i < background_cmd_count; i++) {
    // if execute fails it will free all resources associated with the execution
    // context, so not really much else we can do...
    list.data[i].flags |= EC_NO_WAIT;
    pids[i] = execute(list.data[i]);
  }

  int status = execute(list.data[list.length - 1]);
  for (size_t i = 0; i < background_cmd_count; i++) {
    if (pids[i] != -1) {
      status = status || wait_process(pids[i]);
    }
  }

  free(pids);
  VECTOR_DESTROY(list);

  return status;

error:
  for (size_t i = 0; i < list.length; i++) {
    execution_context_destroy(list.data + i);
  }
  return -1;
}

int evaluate(const TokenList *tokens) {
  EvalState s = {.tokens = tokens, .current = 0};

  int status = 0;

  while (1) {
    Command *command = parse_command(&s);

    if (command == NULL) {
      return -1;
    }

    status = dispatch_external_command(command);

    set_exit_code_var(status);

    if (is_at_end(&s)) {
      break;
    }

    // go again!
    if (match(&s, TK_SEMI)) {
      continue;
    }

    if (match(&s, TK_LOGICAL_OR)) {
      // again!
      if (status != 0) {
        continue;
      }
      // short circuit
      break;
    }

    if (match(&s, TK_LOGICAL_AND)) {
      // again!
      if (status == 0) {
        continue;
      }
      // short circuit
      break;
    }

    error_f("rash: expected some type of command separator (i.e. `||`, `&&`, or `|`) but found %s.\n", get_token_kind_name(peek(&s)->kind));
    return -1;
  }

  return status;
}
