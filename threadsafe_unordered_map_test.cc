#include <gtest/gtest.h>

#include "threadsafe_unordered_map.h"

namespace threadsafe_unordered_map_test
{
    TEST(ThreadsafeUnorderedMapTest, OperatorSubscriptTest)
    {
        threadsafe_unordered_map<int, int> map;
        ASSERT_EQ(1, map[0] = 1);
        ASSERT_EQ(1, map[0]);
    }

    TEST(ThreadsafeUnorderedMapTest, GetOrAddTest)
    {
        threadsafe_unordered_map<int, int> map;
        ASSERT_EQ(1, map.get_or_add(0, 1));
        ASSERT_EQ(1, map[0]);
        ASSERT_EQ(1, map.get_or_add(0, 2));
    }

    TEST(ThreadsafeUnorderedMapTest, TryAddTest)
    {
        threadsafe_unordered_map<int, int> map;
        ASSERT_TRUE(map.try_add(0, 1));
        ASSERT_EQ(1, map[0]);
        ASSERT_FALSE(map.try_add(0, 1));
        ASSERT_EQ(1, map[0]);
    }

    TEST(ThreadsafeUnorderedMapTest, TryGetTest)
    {
        int value;
        threadsafe_unordered_map<int, int> map;
        ASSERT_FALSE(map.try_get(0, value));
        map[0] = 1;
        ASSERT_TRUE(map.try_get(0, value));
        ASSERT_EQ(1, value);
    }

    TEST(ThreadsafeUnorderedMapTest, EraseTest)
    {
        int value;
        threadsafe_unordered_map<int, int> map;
        ASSERT_EQ(0, map.erase(0));
        map[0] = 1;
        ASSERT_EQ(1, map.erase(0));
        ASSERT_FALSE(map.try_get(0, value));
    }
}
