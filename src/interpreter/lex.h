#ifndef LEX_H
#define LEX_H

#include <stdint.h>

typedef enum {
  STRING = 0,
  STDIN_REDIR, // '<' used to redirect stdin from a file
  STDIN_REDIR_STRING, // '<<<' used to redirect stdin from a string
  STDOUT_REDIR, // '>' used to redirect stdout to a file, replace file contents
  STDOUT_REDIR_APPEND, // '>>' used to redirect stdout to a file, append to file contents
  STDERR_REDIR, // '2>' used to redirect stderr to a file, replace file contents
  STDERR_REDIR_APPEND, // '2>>' used to redirect stderr to a file, append to file contents
  PIPE, // '|' used to link one programs stdout to anothers stdin
  GLOB, // any string with a glob (i.e. *, ?, [^x], [!x]) for the time being, this remains unimplemented
  SEMI, // ';' used to run two commands sequentially
  LOGICAL_AND, // '&&' used to run two commands sequencially, but only run the second if the first is successful
  LOGICAL_OR, // '||' used to run two commands sequencially, resulting status code is the logical or of the two commands status 
  AMP, // '&' run a program in the background
  END // end a sequence of tokens
} token_type_t;

extern const char *const TOKEN_NAMES[];

typedef struct {
  token_type_t type;
  void *data;
} token_t;

token_t *lex(const uint8_t * source);

void free_tokens(token_t** tokens);

#endif
