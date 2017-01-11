#include <chrono>

#include <gtest/gtest.h>

#include "threadsafe_blocking_queue.h"

namespace threadsafe_blocking_queue_test
{
    TEST(ThreadsafeBlockingQueueTest, EmptyTest)
    {
        threadsafe_blocking_queue<int> queue;
        ASSERT_TRUE(queue.empty());
        queue.push(0);
        ASSERT_FALSE(queue.empty());
    }

    TEST(ThreadsafeBlockingQueueTest, WaitAndPopTest)
    {
        threadsafe_blocking_queue<int> queue;
        queue.push(0);
        ASSERT_EQ(0, queue.wait_and_pop());
        ASSERT_TRUE(queue.empty());
    }

    TEST(ThreadsafeBlockingQueueTest, TimedWaitAndPopTest)
    {
        int value;
        threadsafe_blocking_queue<int> queue;
        ASSERT_FALSE(queue.timed_wait_and_pop(value, std::chrono::milliseconds(1)));
        queue.push(0);
        ASSERT_TRUE(queue.timed_wait_and_pop(value, std::chrono::milliseconds(1000)));
        ASSERT_TRUE(queue.empty());
        ASSERT_EQ(0, value);
    }
}
