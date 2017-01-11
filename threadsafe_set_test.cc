#include <gtest/gtest.h>

#include "threadsafe_set.h"

namespace threadsafe_set_test
{
    TEST(ThreadsafeSetTest, EmptyTest)
    {
        threadsafe_set<int> set;
        ASSERT_TRUE(set.empty());
        set.insert(0);
        ASSERT_FALSE(set.empty());
    }

    TEST(ThreadsafeSetTest, SizeTest)
    {
        threadsafe_set<int> set;
        ASSERT_EQ(0, set.size());
        for (auto i = 0; i < 2; ++i)
        {
            set.insert(i);
            ASSERT_EQ(i + 1, set.size());
        }
    }

    TEST(ThreadsafeSetTest, InsertTest)
    {
        threadsafe_set<int> set;
        set.insert(0);
        set.insert(0);
        ASSERT_EQ(1, set.size());
    }

    TEST(ThreadsafeSetTest, EraseTest)
    {
        threadsafe_set<int> set;
        set.insert(0);
        set.erase(0);
        ASSERT_EQ(0, set.count(0));
    }

    TEST(ThreadsafeSetTest, CountTest)
    {
        threadsafe_set<int> set;
        set.insert(0);
        ASSERT_EQ(1, set.count(0));
        ASSERT_EQ(0, set.count(1));
    }
}
