#ifndef VECTOR_H
#define VECTOR_H

/*
rash's type generic vector implementation.

Unlike the rest of rash's code, this vector is public domain, you may use it
anyway you desire.

VECTOR(type): expands to a anonymous struct that contains the vectors contents.

e.g.
```
VECTOR(int) my_vector;

typedef VECTOR(int) int_vector;

int_vector my_other_vector;
```

VECTOR_INIT(vector): initializes the vector with a capacity of 16.
VECTOR_INIT(vector, capacity): initilizes the vector with whatever capacity you
want.

e.g.
```
VECTOR(int) my_vector;
VECTOR_INIT(my_vector); // capacity of 16

VECTOR(float) my_floats;
VECTOR_INIT(my_floats, 64); // capacity of 64
```

VECTOR_PUSH(vector, value): append value to the end of the vector.

e.g.
```
VECTOR(float) my_floats;
VECTOR_INIT(my_floats);

VECTOR_PUSH(my_floats, 1.2f); // adds 1.2f at position zero in the vector
```

VECTOR_POP(vector): removes the last value from the vector, and returns the item
removed.

e.g.
```
VECTOR(float) my_floats;
VECTOR_INIT(my_floats);

VECTOR_PUSH(my_floats, 1.2f);

float last_item = VECTOR_POP(my_floats); // vector is now empty
last_item == 1.2f; // true
```

VECTOR_AT(vector, index): get the value in the vector at the index provided.

e.g.
```
VECTOR(float) my_floats;
VECTOR_INIT(my_floats);

VECTOR_PUSH(my_floats, 1.2f);

float last_item = VECTOR_AT(my_floats, 0);
last_item == 1.2f; // true
```

VECTOR_DESTROY(vector): frees all the memory associated with the vector, you
cannot use this vector after calling VECTOR_DESTROY.

e.g.
```
VECTOR(float) my_floats;
VECTOR_INIT(my_floats);

VECTOR_PUSH(my_floats, 1.2f);

VECTOR_DESTROY(my_floats);

VECTOR_PUSH(my_floats, 1.3f); // segmentation fault (or memory corruption)
```

VECTOR_CLEAR(vector): clears out all items in a vector, but it is still usable.

e.g.
```
VECTOR(float) my_floats;
VECTOR_INIT(my_floats);

VECTOR_PUSH(my_floats, 1.2f);

VECTOR_DESTROY(my_floats);

VECTOR_PUSH(my_floats, 1.3f);
VECTOR_AT(my_floats, 0) // 1.3f
```

Other Examples:

```
// a string to can add characters to
VECTOR(char) string;
VECTOR_INIT(string);

VECTOR_PUSH(string, 'h');
VECTOR_PUSH(string, 'i');
VECTOR_PUSH(string, '\0');

printf("%s", string.data);
```
*/

#include <assert.h>
#include <stdlib.h>

#define VECTOR_DEFAULT_SIZE 16

#define VECTOR(type)                                                           \
  struct {                                                                     \
    size_t _capacity;                                                          \
    size_t length;                                                             \
    type *data;                                                                \
  }

#define USE_VECTOR_INIT_INSTEAD_OF_THIS(vector, capacity, ...)                 \
  do {                                                                         \
    (vector)._capacity = capacity;                                             \
    (vector).length = 0;                                                       \
    if (capacity != 0)                                                         \
      (vector).data = malloc(sizeof(*(vector).data) * capacity);               \
    else                                                                       \
      (vector).data = NULL;                                                    \
  } while (0)

#define VECTOR_INIT(...)                                                       \
  USE_VECTOR_INIT_INSTEAD_OF_THIS(__VA_ARGS__, VECTOR_DEFAULT_SIZE, unused)

#define VECTOR_PUSH(vector, value)                                             \
  do {                                                                         \
    if ((vector)._capacity <= (vector).length) {                               \
      if ((vector)._capacity == 0)                                             \
        (vector)._capacity = 1;                                                \
      else                                                                     \
        (vector)._capacity *= 2;                                               \
      (vector).data =                                                          \
          realloc((vector).data, sizeof(*(vector).data) * (vector)._capacity); \
    }                                                                          \
    (vector).data[(vector).length] = value;                                    \
    (vector).length++;                                                         \
  } while (0)

// no length checks are done for this macro, always check the length is not zero
// before using vector_pop
#define VECTOR_POP(vector) (vector).data[--(vector).length]

#define VECTOR_AT(vector, index) (vector).data[index]

#define VECTOR_DESTROY(vector)                                                 \
  do {                                                                         \
    free((vector).data);                                                       \
    (vector).data = NULL;                                                      \
  } while (0)

#define VECTOR_CLEAR(vector) (vector).length = 0

#endif
