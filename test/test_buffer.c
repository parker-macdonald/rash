#include "test.h"

#include "lib/buffer.h"
#include <string.h>

TEST_SUITE("buffer", {
  TEST("from_cstr", {
    Buffer buffer = buffer_from_cstr("wasssup");

    ASSERT_EQ(buffer.length, 7)
    ASSERT_EQ(memcmp(buffer.void_ptr, "wasssup", 7), 0);

    buffer_destroy(&buffer);
  });
  
  TEST("append", {
    Buffer buffer = {0};
    buffer_append_cstr(&buffer, "abcdefgh");

    ASSERT_EQ(buffer.length, 8)
    ASSERT_EQ(memcmp(buffer.void_ptr, "abcdefgh", 8), 0);

    buffer_append_byte(&buffer, 'i');

    ASSERT_EQ(buffer.length, 9)
    ASSERT_EQ(memcmp(buffer.void_ptr, "abcdefghi", 9), 0);

    buffer_destroy(&buffer);
  });

  TEST("remove", {
    Buffer buffer = buffer_from_cstr("abcdefg");
    buffer_remove_n(&buffer, 0, 3);

    ASSERT_EQ(buffer.length, 4)
    ASSERT_EQ(memcmp(buffer.void_ptr, "defg", 4), 0);

    buffer_remove_n(&buffer, buffer.length - 2, 2);

    ASSERT_EQ(buffer.length, 2)
    ASSERT_EQ(memcmp(buffer.void_ptr, "de", 2), 0);

    buffer_destroy(&buffer);
  });
  
  TEST("insert", {
    Buffer buffer = buffer_from_cstr("abcdefgh");
    buffer_insert_cstr(&buffer, 0, "yo ");
    
    ASSERT_EQ(buffer.length, 11);
    ASSERT_EQ(memcmp(buffer.void_ptr, "yo abcdefgh", 11), 0);
    
    buffer_insert_cstr(&buffer, buffer.length, " :3333");

    ASSERT_EQ(buffer.length, 17);
    ASSERT_EQ(memcmp(buffer.void_ptr, "yo abcdefgh :3333", 17), 0);

    buffer_destroy(&buffer);
  });

  TEST("insert", {
    Buffer buffer = buffer_from_cstr("abcdefgh");
    buffer_insert_cstr(&buffer, 0, "yo ");
    
    ASSERT_EQ(buffer.length, 11);
    ASSERT_EQ(memcmp(buffer.void_ptr, "yo abcdefgh", 11), 0);
    
    buffer_insert_cstr(&buffer, buffer.length, " :3333");

    ASSERT_EQ(buffer.length, 17);
    ASSERT_EQ(memcmp(buffer.void_ptr, "yo abcdefgh :3333", 17), 0);

    buffer_destroy(&buffer);
  });

  TEST("slice", {
    Buffer buffer = buffer_from_cstr("abcdefgh");

    Buffer slice1 = buffer_slice(&buffer, 0, 4);
    
    ASSERT_EQ(slice1.length, 4);
    ASSERT_EQ(memcmp(buffer.void_ptr, "abcd", 4), 0);

    Buffer slice2 = buffer_slice(&buffer, 5, buffer.length);

    ASSERT_EQ(slice2.length, 3);
    ASSERT_EQ(memcmp(slice2.void_ptr, "fgh", 3), 0);

    buffer_destroy(&buffer);
    buffer_destroy(&slice1);
    buffer_destroy(&slice2);
  });

  TEST("compare", {
    {
      Buffer buffer1 = buffer_from_cstr("abcdefgh");
      Buffer buffer2 = buffer_from_cstr("abcdefgh");
      
      ASSERT_EQ(buffer_compare(&buffer1, &buffer2), 0);
      buffer_destroy(&buffer1);
      buffer_destroy(&buffer2);
    }

    {
      Buffer buffer1 = buffer_from_cstr("bbcdefgh");
      Buffer buffer2 = buffer_from_cstr("abcdefgh");
      
      ASSERT_EQ(buffer_compare(&buffer1, &buffer2), 1);
      buffer_destroy(&buffer1);
      buffer_destroy(&buffer2);
    }

    {
      Buffer buffer1 = buffer_from_cstr("abcd");
      Buffer buffer2 = buffer_from_cstr("abcdefgh");
      
      ASSERT_EQ(buffer_compare(&buffer1, &buffer2), -1);
      buffer_destroy(&buffer1);
      buffer_destroy(&buffer2);
    }

    {
      Buffer buffer1 = buffer_from_cstr("abcdefgh");
      Buffer buffer2 = buffer_from_cstr("abcd");
      
      ASSERT_EQ(buffer_compare(&buffer1, &buffer2), 1);
      buffer_destroy(&buffer1);
      buffer_destroy(&buffer2);
    }
  });
})
