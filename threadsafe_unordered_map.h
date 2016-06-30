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

    V &get_or_add(const K &key, V value)
    {
        std::lock_guard<std::mutex> lock(access_lock);
        return table.count(key) > 0
            ? table[key]
            : table[key] = value;
    }

    bool try_add(const K &key, V &value)
    {
        std::lock_guard<std::mutex> lock(access_lock);
        if (table.count(key) > 0)
        {
            return false;
        }

        table[key] = value;
        return true;
    }

    bool try_get(const K &key, V &value)
    {
        std::lock_guard<std::mutex> lock(access_lock);
        if (table.count(key) > 0)
        {
            value = table[key];
            return true;
        }

        return false;
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
