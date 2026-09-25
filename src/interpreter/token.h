#ifndef INTERPRETER_TOKEN_H
#define INTERPRETER_TOKEN_H

#include "lib/buffer.h"
#include "lib/vector.h"

// the string `echo "shell lvl: ${SHLVL}"; echo c files: src/*.c` lexed looks like:

// TK_ARG_STRING("echo")
// TK_ARG_END
// TK_ARG_STRING("shell lvl: ")
// TK_ARG_ENV("SHLVL")
// TK_ARG_END
// TK_SEMI
// TK_ARG_STRING("echo")
// TK_ARG_END
// TK_ARG_STRING("c")
// TK_ARG_END
// TK_ARG_STRING("files:")
// TK_ARG_END
// TK_ARG_STRING("src/")
// TK_ARG_WILDCARD
// TK_ARG_STRING(".c")
// TK_ARG_END

// argument tokens (tokens that have an impact on the argv of run commands) all begin with TK_ARG
// TK_ARG_END tokens are special tokens used to end a list of connected argument tokens all the 
// connected argument tokens make up a single element of argv in a command.
// for example `~/$EXAMPLE_FOLDER` will lex to:

// TK_ARG_TILDE("")
// TK_ARG_STRING("/")
// TK_ARG_ENV("EXAMPLE_FOLDER")

// when this gets evaluated these operations will be preformed one after another appending to a string
// that contains the final element of argv. that element might look like `/root/example`

typedef enum {
  // '<' used to redirect stdin from a file
  TK_STDIN_REDIR,
  // '<<<' used to redirect stdin from a string
  TK_STDIN_REDIR_STRING,
  // '>' used to redirect stdout to a file, replace file contents
  TK_STDOUT_REDIR,
  // '>>' used to redirect stdout to a file, append to file contents
  TK_STDOUT_REDIR_APPEND,
  // '2>' used to redirect stderr to a file, replace file contents
  TK_STDERR_REDIR,
  // '2>>' used to redirect stderr to a file, append to file contents
  TK_STDERR_REDIR_APPEND,
    // '&>' used to redirect stdout and stderr to a file, replace file contents
  TK_STDOUT_ERR_REDIR,
  // '&>>' used to redirect stdout and stderr to a file, append to file contents
  TK_STDOUT_ERR_REDIR_APPEND,
  // '|' used to link one programs stdout to anothers stdin
  TK_PIPE,
  // '||' used to run two commands sequencially, but only run the second if the
  // first is unsuccessful
  TK_LOGICAL_OR,
  // ';' used to run two commands sequentially
  TK_SEMI,
  // '&&' used to run two commands sequencially, but only run the second if the
  // first is successful
  TK_LOGICAL_AND,
  // '&' run a program in the background
  TK_AMP,
  // '!' marking the last argument is a macro
  TK_MACRO,

  TK_ARG_STRING,
  // '*' matches zero or more characters while globbing
  TK_ARG_WILDCARD,
  // '**' matches zero or more characters while globbing and matches multiple dirs
  TK_ARG_DOUBLESTAR,
  // environment variable to be expanded
  TK_ARG_ENV,
  // shell expression to be expanded i.e. {1 + 2}
  TK_ARG_SHELL_EXPR,
  // subshell i.e. $(example string)
  TK_ARG_SUBSHELL,
  // '~' used for home folder expansion
  TK_ARG_TILDE,
  // marks the end of a list of argument tokens
  TK_ARG_END,

  // this token will never be the result of a lex and is used a sentinal value
  TK_NONE
} TokenKind;

// NOTE: TK_ARG_END is not an argument token
#define IS_ARGUMENT_TOKEN(kind) ( \
  (kind) == TK_ARG_STRING || \
  (kind) == TK_ARG_WILDCARD || \
  (kind) == TK_ARG_DOUBLESTAR || \
  (kind) == TK_ARG_ENV || \
  (kind) == TK_ARG_SHELL_EXPR || \
  (kind) == TK_ARG_SUBSHELL || \
  (kind) == TK_ARG_TILDE \
)

#define IS_BUFFER_TOKEN(kind) ( \
  (kind) == TK_ARG_STRING || \
  (kind) == TK_ARG_ENV || \
  (kind) == TK_ARG_SHELL_EXPR || \
  (kind) == TK_ARG_SUBSHELL || \
  (kind) == TK_ARG_TILDE \
)

typedef struct {
  TokenKind kind;
  // only populated if IS_BUFFER_TOKEN(token) is true
  Buffer buffer;
} Token;

typedef VECTOR(Token) TokenList;

void token_destroy(Token *token);

void token_list_destroy(TokenList *list);

#endif
