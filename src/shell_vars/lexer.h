#ifndef SHELL_VARS_LEXER_H
#define SHELL_VARS_LEXER_H

#include "lib/slice.h"
#include "shell_vars/token.h"

TokenList lex_shell_expr(const Slice *source);

#endif
