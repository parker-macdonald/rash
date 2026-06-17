#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdlib.h>

#define LL(type)                                                               \
  struct {                                                                     \
    struct {                                                                   \
      void *p_next, *p_prev;                                                   \
      type value;                                                              \
    } *head, *tail;                                                            \
    size_t length;                                                             \
  }

#define LL_PUSH(list, _value)                                                  \
  do {                                                                         \
    if ((list).head == NULL) {                                                 \
      __typeof((list).head) _node = malloc(sizeof(*(list).head));              \
      _node->p_next = NULL;                                                    \
      _node->p_prev = NULL;                                                    \
      _node->value = (_value);                                                 \
      (list).head = _node;                                                     \
      (list).tail = _node;                                                     \
      (list).length = 1;                                                       \
      break;                                                                   \
    }                                                                          \
                                                                               \
    __typeof((list).head) _node = malloc(sizeof(*(list).head));                \
    _node->p_next = NULL;                                                      \
    _node->p_prev = (list).tail;                                               \
    _node->value = (_value);                                                   \
                                                                               \
    (list).tail->p_next = _node;                                               \
    (list).tail = _node;                                                       \
  } while (0)

#define LL_POP(list)                                                           \
  do {                                                                         \
    if ((list).head == NULL) {                                                 \
      break;                                                                   \
    }                                                                          \
                                                                               \
    if ((list).head == (list).tail) {                                          \
      free((list).head);                                                       \
      (list).head = NULL;                                                      \
      (list).tail = NULL;                                                      \
      (list).length = 0;                                                       \
      break;                                                                   \
    }                                                                          \
                                                                               \
    (list).tail = (list).tail->p_prev;                                         \
    free((list).tail->p_next);                                                 \
    (list).tail->p_next = NULL;                                                \
  } while (0)

#define LL_HEAD(list, sentinal) ((list).head ? (list).head->value : (sentinal))

#define LL_HEAD_UNCHECKED(list) ((list).head->value)

#define LL_END(list, sentinal) ((list).tail ? (list).tail->value : (sentinal))

#define LL_END_UNCHECKED(list) ((list).tail->value)

#define LL_ITER_CREATE(name, list)                                             \
  struct {                                                                     \
    __typeof((list).head) curr;                                                \
    __typeof((list)) *_list;                                                   \
  } name = {.curr = (list).head, ._list = &(list)}

#define LL_ITER_CURR(iter) ((iter).curr ? &(iter).curr->value : NULL)

#define LL_ITER_NEXT(iter)                                                     \
  do {                                                                         \
    if ((iter).curr == NULL || (iter).curr->p_next == NULL) {                  \
      (iter).curr = NULL;                                                      \
      break;                                                                   \
    }                                                                          \
                                                                               \
    (iter).curr = (iter).curr->p_next;                                         \
  } while (0)

#define LL_ITER_REMOVE(iter)                                                   \
  do {                                                                         \
    if ((iter).curr == NULL) {                                                 \
      break;                                                                   \
    }                                                                          \
                                                                               \
    __typeof((iter).curr) _prev = (iter).curr->p_prev;                         \
    __typeof((iter).curr) _next = (iter).curr->p_next;                         \
                                                                               \
    if ((iter).curr == (iter)._list->head) {                                   \
      (iter)._list->head = _next;                                              \
    }                                                                          \
    if ((iter).curr == (iter)._list->tail) {                                   \
      (iter)._list->tail = _prev;                                              \
    }                                                                          \
    if (_prev != NULL) {                                                       \
      _prev->p_next = _next;                                                   \
    }                                                                          \
    if (_next != NULL) {                                                       \
      _next->p_prev = _prev;                                                   \
    }                                                                          \
    free((iter).curr);                                                         \
    (iter).curr = _next;                                                       \
  } while (0)

#define LL_DESTROY(list)                                                       \
  do {                                                                         \
    __typeof((list).head) _node = (list).head;                                 \
                                                                               \
    while (_node != NULL) {                                                    \
      __typeof(_node) _temp = _node->p_next;                                   \
      free(_node);                                                             \
      _node = _temp;                                                           \
    }                                                                          \
  } while (0)

#endif
