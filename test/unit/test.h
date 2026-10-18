#ifndef TEST_H
#define TEST_H

#include <stdio.h>

#define PASS(suite_name, test_name)                                            \
  printf("\033[32m[PASS %s:%s]\033[0m\n", suite_name, test_name)
#define FAIL(suite_name, test_name, reason)                                    \
  printf("\033[31m[FAIL %s:%s: %s]\033[0m\n", suite_name, test_name, reason)

#define ASSERT(cond, reason)                                                   \
  if (!(cond)) {                                                               \
    FAIL(_reserved_suite, _reserved_name, reason);                             \
    break;                                                                     \
  }

#define ASSERT_EQ(a, b) ASSERT((a) == (b), #a " does not equal " #b)
#define ASSERT_NEQ(a, b) ASSERT((a) != (b), #a " equals " #b)
#define ASSERT_TRUE(a) ASSERT((a), #a " is not true")
#define ASSERT_FALSE(a) ASSERT(!(a), #a " is not false")

#define TEST(test_name, code)                                                  \
  do {                                                                         \
    char *_reserved_name = test_name;                                          \
    _reserved_test_count++;                                                    \
    code;                                                                      \
    _reserved_passed++;                                                        \
    PASS(_reserved_suite, _reserved_name);                                     \
  } while (0)

#define TEST_SUITE(suite_name, code)                                           \
  int main(void) {                                                             \
    char *_reserved_suite = suite_name;                                        \
    int _reserved_passed = 0;                                                  \
    int _reserved_test_count = 0;                                              \
    code;                                                                      \
    printf(                                                                    \
        "%d / %d tests passed. %.3f%% passing.\n",                             \
        _reserved_passed,                                                      \
        _reserved_test_count,                                                  \
        (double)_reserved_passed / (double)_reserved_test_count * 100.0        \
    );                                                                         \
    return _reserved_passed != _reserved_test_count;                           \
  }

#endif
