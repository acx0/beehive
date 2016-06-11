#ifndef THREADSAFE_SET_H
#define THREADSAFE_SET_H

#include <mutex>
#include <set>

template <typename T>
class threadsafe_set
{
public:
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(access_lock);
        return data.empty();
    }

    typename std::set<T>::size_type size() const
    {
        std::lock_guard<std::mutex> lock(access_lock);
        return data.size();
    }

    void insert(const T &value)
    {
        std::lock_guard<std::mutex> lock(access_lock);
        data.insert(value);
    }

    typename std::set<T>::size_type erase(const T &value)
    {
        std::lock_guard<std::mutex> lock(access_lock);
        return data.erase(value);
    }

    typename std::set<T>::size_type count(const T &value) const
    {
        std::lock_guard<std::mutex> lock(access_lock);
        return data.count(value);
    }

private:
    mutable std::mutex access_lock;
    std::set<T> data;
};

#endif
