#ifndef THREADSAFE_UNORDERED_MAP_H
#define THREADSAFE_UNORDERED_MAP_H

#include <functional>
#include <mutex>
#include <unordered_map>

template <typename K, typename V, typename Hash = std::hash<K>>
class threadsafe_unordered_map
{
public:
    V &operator[](const K &key)
    {
        std::lock_guard<std::mutex> lock(access_lock);
        return table[key];
    }

    typename std::unordered_map<K, V, Hash>::size_type erase(const K &key)
    {
        std::lock_guard<std::mutex> lock(access_lock);
        return table.erase(key);
    }

private:
    mutable std::mutex access_lock;
    std::unordered_map<K, V, Hash> table;
};

#endif
