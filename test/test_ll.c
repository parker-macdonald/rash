#include "lib/linked_list.h"
#include "test.h"

TEST_SUITE("linked_list", {
  TEST("push", {
    LL(int) list = {0};
    
    LL_PUSH(list, 5);

    ASSERT_EQ(LL_END(list, -1), 5);

    LL_PUSH(list, 6);

    ASSERT_EQ(LL_END(list, -1), 6);

    LL_DESTROY(list);
  });
  
  TEST("pop", {
    LL(int) list = {0};
    
    LL_PUSH(list, 5);
    LL_PUSH(list, 6);

    LL_POP(list);

    ASSERT_EQ(LL_END(list, -1), 5);

    LL_POP(list);

    ASSERT_EQ(LL_END(list, -1), -1);

    LL_DESTROY(list);
  });

  TEST("iter", {
    LL(int) list = {0};

    for (int i = 0; i < 10; i++) {
      LL_PUSH(list, i);
    }

    LL_ITER_CREATE(iter, list);

    for (int i = 0; i < 10; i++) {
      ASSERT_NEQ(LL_ITER_CURR(iter), NULL);

      ASSERT_EQ(*LL_ITER_CURR(iter), i);

      LL_ITER_NEXT(iter);
    }

    ASSERT_EQ(LL_ITER_CURR(iter), NULL);
    LL_DESTROY(list);
  });

  TEST("remove", {
    LL(int) list = {0};

    LL_PUSH(list, 1);
    LL_PUSH(list, 2);
    LL_PUSH(list, 3);
    LL_PUSH(list, 4);
    LL_PUSH(list, 5);

    LL_ITER_CREATE(iter, list);

    ASSERT_EQ(*LL_ITER_CURR(iter), 1);

    LL_ITER_REMOVE(iter);
    ASSERT_EQ(*LL_ITER_CURR(iter), 2);

    LL_ITER_NEXT(iter);
    
    LL_ITER_NEXT(iter);
    ASSERT_EQ(*LL_ITER_CURR(iter), 4);
    LL_ITER_REMOVE(iter);
    ASSERT_EQ(*LL_ITER_CURR(iter), 5);

    LL_ITER_REMOVE(iter);
    ASSERT_EQ(LL_ITER_CURR(iter), NULL);

    LL_DESTROY(list);
  });
})
