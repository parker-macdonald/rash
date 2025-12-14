#ifndef SHELL_VARS_H
#define SHELL_VARS_H

/**
 * @brief set a shell variable given a key and a value. in case it isn't
 * obvious, the data passed in is cloned.
 * @param key the key of the shell variable.
 * @param value the value of the shell variable.
 */
void var_set(const char *const key, const char *const value);

/**
 * @brief get a shell variable given a key.
 * @param key the key of the variable
 * @return returns the associated value or null
 */
const char *var_get(const char *const key);

/**
 * @brief unset a shell variable given a key
 * @param key the key of the variable
 * @return returns 1 if no variable was removed, returns 0 otherwise.
 */
int var_unset(const char *const key);

/**
 * @brief prints all shell variables in a list.
 */
void var_print(void);

#endif
