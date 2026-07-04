#ifndef SHELL_VARS_EVAL_H
#define SHELL_VARS_EVAL_H

#include "shell_vars.h"
#include "shell_vars/token.h"

ShellVar *evaluate_tokens(const TokenList *tokens);

#endif
