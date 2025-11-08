#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <unordered_map>
#include <list>
#include <string>
#include <mutex>

class LRUCache
{
    size_t capacity;
    std::list<std::pair<std::string, std::string>> item_list;
    std::unordered_map<std::string, decltype(item_list.begin())> item_map;
    std::mutex mtx;

public:
    LRUCache(size_t cap) : capacity(cap) {}

    bool get(const std::string &key, std::string &value)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = item_map.find(key);
        if (it == item_map.end())
            return false;
        item_list.splice(item_list.begin(), item_list, it->second);
        value = it->second->second;
        return true;
    }

    void put(const std::string &key, const std::string &value)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = item_map.find(key);
        if (it != item_map.end())
        {
            it->second->second = value;
            item_list.splice(item_list.begin(), item_list, it->second);
            return;
        }
        if (item_list.size() >= capacity)
        {
            auto last = item_list.back();
            item_map.erase(last.first);
            item_list.pop_back();
        }
        item_list.emplace_front(key, value);
        item_map[key] = item_list.begin();
    }

    void erase(const std::string &key)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = item_map.find(key);
        if (it != item_map.end())
        {
            item_list.erase(it->second);
            item_map.erase(it);
        }
    }
};

#endif
