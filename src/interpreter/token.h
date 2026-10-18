#ifndef INTERPRETER_TOKEN_H
#define INTERPRETER_TOKEN_H

#include "lib/buffer.h"
#include "lib/vector.h"

// the string `echo "shell lvl: ${SHLVL}"; echo c files: src/*.c` lexed looks like:

// TK_ARG_BEING
// TK_ARG_STRING("echo")
// TK_ARG_END
// TK_ARG_BEGIN
// TK_ARG_STRING("shell lvl: ")
// TK_ARG_ENV("SHLVL")
// TK_ARG_END
// TK_SEMI
// TK_ARG_BEGIN
// TK_ARG_STRING("echo")
// TK_ARG_END
// TK_ARG_BEGIN
// TK_ARG_STRING("c")
// TK_ARG_END
// TK_ARG_BEGIN
// TK_ARG_STRING("files:")
// TK_ARG_END
// TK_ARG_BEGIN
// TK_ARG_STRING("src/")
// TK_ARG_WILDCARD
// TK_ARG_STRING(".c")
// TK_ARG_END

// argument tokens (tokens that have an impact on the argv of run commands) all begin with TK_ARG
// TK_ARG_END tokens are special tokens used to end a list of connected argument tokens all the 
// connected argument tokens make up a single element of argv in a command.
// for example `~/$EXAMPLE_FOLDER` will lex to:

// TK_ARG_BEGIN
// TK_ARG_TILDE("")
// TK_ARG_STRING("/")
// TK_ARG_ENV("EXAMPLE_FOLDER")
// TK_ARG_END

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
  
  // marks the start and end of a list of argument tokens
  TK_ARG_BEGIN,
  TK_ARG_END,

  TK_ARG_STRING,
  // '*' matches zero or more characters while globbing
  TK_ARG_WILDCARD,
  // '**' matches zero or more characters while globbing and matches multiple dirs
  TK_ARG_DOUBLESTAR,
  // environment variable to be expanded
  TK_ARG_ENV,
  // subshell i.e. $(example string)
  TK_ARG_SUBSHELL,
  // '~' used for home folder expansion
  TK_ARG_TILDE,

  // this token will never be the result of a lex and is used a sentinal value
  TK_NONE,

  // tokens for shell expressions
  TK_SE_BEGIN,
  TK_SE_END,

  TK_SE_NONE,
  
  TK_SE_ADD, // +
  TK_SE_SUB, // -
  TK_SE_MUL, // *
  TK_SE_POW, // **
  TK_SE_DIV, // /
  TK_SE_MOD, // %

  TK_SE_EQ, // ==
  TK_SE_NEQ, // !=
  TK_SE_GT, // >
  TK_SE_LT, // <
  TK_SE_GTE, // >=
  TK_SE_LTE, // <=
  TK_SE_NOT, // !

  // used for ternary operator
  TK_SE_QUESTION, // ?
  TK_SE_COLON, // :

  TK_SE_O_PAREN, // (
  TK_SE_C_PAREN, // )

  TK_SE_COMMA, // ,

  TK_SE_STRING_LIT,
  TK_SE_NUMBER_LIT,
  TK_SE_NULL_LIT,
  TK_SE_TRUE_LIT,
  TK_SE_FALSE_LIT,

  TK_SE_IDENTIFIER
} TokenKind;

#define IS_BUFFER_TOKEN(kind) ( \
  (kind) == TK_ARG_STRING || \
  (kind) == TK_ARG_ENV || \
  (kind) == TK_ARG_SUBSHELL || \
  (kind) == TK_ARG_TILDE || \
  (kind) == TK_SE_STRING_LIT || \
  (kind) == TK_SE_IDENTIFIER \
)

typedef struct {
  TokenKind kind;
  union {
    // used for TK_SE_NUMBER_LIT
    double number;
    // only populated if IS_BUFFER_TOKEN(token) is true
    Buffer buffer;
  };
} Token;

typedef VECTOR(Token) TokenList;

void token_destroy(Token *token);

void token_list_destroy(TokenList *list);

const char *get_token_kind_name(TokenKind kind);

#endif
