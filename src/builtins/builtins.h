#ifndef BUILTINS_H
#define BUILTINS_H

int builtin_cd(char **argv);

int builtin_help(char **argv);

int builtin_exit(char **argv);

int builtin_export(char **argv);

int builtin_history(char **argv);

int builtin_true(char **argv);

int builtin_false(char **argv);

int builtin_pwd(char **argv);

int builtin_fg(char **argv);

int builtin_bg(char **argv);

int builtin_jobs(char **argv);

int builtin_version(char **argv);

int builtin_setvar(char **argv);

int builtin_unsetvar(char **argv);

int builtin_source(char **argv);

int builtin_which(char **argv);

int builtin_var(char **argv);

int builtin_env(char **argv);

int builtin_setenv(char **argv);

int builtin_unsetenv(char **argv);

int builtin_exec(char **argv);

#endif
