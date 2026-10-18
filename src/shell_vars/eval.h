#ifndef SHELL_VARS_EVAL_H
#define SHELL_VARS_EVAL_H

#include "shell_vars.h"
#include "interpreter/evaluate.h"

ShellVar *evaluate_tokens(EvalState *s);

#endif
