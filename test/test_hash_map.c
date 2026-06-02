#include "lib/hash_map.h"
#include "test.h"
#include <string.h>

TEST_SUITE("hash_map", {
  TEST("insert", {
    HashMap map;
    hash_map_init(&map, free);

    hash_map_set(&map, "sup", strdup("yo"));

    char *value = hash_map_get(&map, "sup");

    ASSERT_EQ(strcmp(value, "yo"), 0);

    hash_map_destroy(&map);
  });
  
  TEST("replace", {
    HashMap map;
    hash_map_init(&map, free);

    hash_map_set(&map, "sup", strdup("yo"));

    char *value = hash_map_get(&map, "sup");

    ASSERT_EQ(strcmp(value, "yo"), 0);

    hash_map_set(&map, "sup", strdup("bye"));

    value = hash_map_get(&map, "sup");

    ASSERT_EQ(strcmp(value, "bye"), 0);

    hash_map_destroy(&map);
  });

  TEST("remove", {
    HashMap map;
    hash_map_init(&map, free);

    hash_map_set(&map, "sup", strdup("yo"));

    char *value = hash_map_get(&map, "sup");

    ASSERT_EQ(strcmp(value, "yo"), 0);

    hash_map_remove(&map, "sup");

    value = hash_map_get(&map, "sup");

    ASSERT_EQ(value, NULL);

    hash_map_destroy(&map);
  });

  TEST("add_many", {
    HashMap map;
    hash_map_init(&map, free);

    hash_map_set(&map, "sup", strdup("a"));
    
    hash_map_set(&map, "grrr", strdup("b"));
    
    hash_map_set(&map, "heheheha", strdup("c"));
    
    hash_map_set(&map, "lool", strdup("d"));
    
    hash_map_set(&map, "haha", strdup("e"));
    
    hash_map_set(&map, "fart", strdup("f"));
    
    hash_map_set(&map, "crap", strdup("g"));
    
    hash_map_set(&map, "long long long long long long long long", strdup("h"));
    
    hash_map_set(&map, "pee", strdup("i"));
    
    ASSERT_EQ(strcmp(hash_map_get(&map, "sup"), "a"), 0);
    ASSERT_EQ(strcmp(hash_map_get(&map, "grrr"), "b"), 0);
    ASSERT_EQ(strcmp(hash_map_get(&map, "heheheha"), "c"), 0);
    ASSERT_EQ(strcmp(hash_map_get(&map, "lool"), "d"), 0);
    ASSERT_EQ(strcmp(hash_map_get(&map, "haha"), "e"), 0);
    ASSERT_EQ(strcmp(hash_map_get(&map, "fart"), "f"), 0);
    ASSERT_EQ(strcmp(hash_map_get(&map, "crap"), "g"), 0);
    ASSERT_EQ(strcmp(hash_map_get(&map, "long long long long long long long long"), "h"), 0);
    ASSERT_EQ(strcmp(hash_map_get(&map, "pee"), "i"), 0);
    
    hash_map_destroy(&map);
  });
})
