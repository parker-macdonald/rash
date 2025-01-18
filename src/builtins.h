#ifndef BUILTINS_H
#define BUILTINS_H

enum builtins_t { CD = 0, HELP, EXIT, EXPORT, NUM_OF_BUILTINS };

extern const char *const builtins[NUM_OF_BUILTINS];

int builtin_cd(char **const argv);

int builtin_help(char **const argv);

int builtin_exit(char **const argv);

int builtin_export(char **const argv);

extern int (*builtin_fns[NUM_OF_BUILTINS])(char **const);

#endif
