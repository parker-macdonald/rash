#ifndef BUILTINS_H
#define BUILTINS_H

int builtin_cd(char **const argv);

int builtin_help(char **const argv);

int builtin_exit(char **const argv);

int builtin_export(char **const argv);

int builtin_history(char **const argv);

int builtin_true(char **const argv);

int builtin_false(char **argv);

int builtin_pwd(char **argv);

int builtin_fg(char **argv);

int builtin_bg(char **argv);

int builtin_jobs(char **argv);

int builtin_version(char **argv);

int builtin_setvar(char **argv);

int builtin_unsetvar(char **argv);

#endif
