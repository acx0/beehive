#ifndef THREADSAFE_BLOCKING_QUEUE_H
#define THREADSAFE_BLOCKING_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class threadsafe_blocking_queue
{
public:
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(access_lock);
        return data.empty();
    }

    void push(const T &value)
    {
        std::lock_guard<std::mutex> lock(access_lock);
        data.push(value);
        condition.notify_one();
    }

    T wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(access_lock);
        condition.wait(lock, [this]{ return !data.empty(); });

        auto value = data.front();
        data.pop();

        return value;
    }

private:
    mutable std::mutex access_lock;
    std::queue<T> data;
    std::condition_variable condition;
};

#endif
