#ifndef BUILTINS_H
#define BUILTINS_H

int builtin_cd(char *const *argv);

int builtin_help(char *const *argv);

int builtin_exit(char *const *argv);

int builtin_export(char *const *argv);

int builtin_history(char *const *argv);

int builtin_true(char *const *argv);

int builtin_false(char *const *argv);

int builtin_pwd(char *const *argv);

int builtin_fg(char *const *argv);

int builtin_bg(char *const *argv);

int builtin_jobs(char *const *argv);

int builtin_version(char *const *argv);

int builtin_setvar(char *const *argv);

int builtin_unsetvar(char *const *argv);

int builtin_source(char *const *argv);

int builtin_which(char *const *argv);

int builtin_var(char *const *argv);

int builtin_env(char *const *argv);

int builtin_setenv(char *const *argv);

int builtin_unsetenv(char *const *argv);

int builtin_exec(char *const *argv);

int builtin_eval(char *const *argv);

int builtin_time(char *const *argv);

int builtin_mkdir(char *const *argv);

int builtin_config(char *const *argv);

#endif
