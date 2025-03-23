#ifndef BUILTINS_H
#define BUILTINS_H

int builtin_cd(char **const argv);

int builtin_help(char **const argv);

int builtin_exit(char **const argv);

int builtin_export(char **const argv);

int builtin_history(char** const argv);

#endif
