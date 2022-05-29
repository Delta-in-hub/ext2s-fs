#ifndef __CACHE_H__
#define __CACHE_H__
#include "disk.h"
#include <unordered_map>
#include <list>
#include <string.h>
#include <vector>
#include <queue>
// LRU CACHE FOR DISK
class Cache
{
private:
    Disk &_disk;
    const unsigned _capacity; // LRU CACHE CAPACITY

    struct cache_item
    {
        uint8_t data[BLOCK_SIZE];
        size_t block_idx;
        bool dirty;
        cache_item()
        {
            memset(data, 0, BLOCK_SIZE);
            block_idx = -1;
            dirty = false;
        }
    };
    void _write_item_back(cache_item &item)
    {
        if (item.dirty)
        {
            _disk.write_block(item.block_idx, item.data);
            item.dirty = false;
        }
    }
    std::vector<cache_item> _cache;
    std::queue<size_t> _free_postion;

    std::list<size_t /*position in _cache*/> _lru_list; // Most Recently Used List , the back one is LRU.
    std::unordered_map<size_t /*block_index*/, decltype(_lru_list.begin())> _lru_map;

    void _update(decltype(_lru_list.begin()) &it)
    {
        auto pos = *it;
        _lru_list.erase(it);
        _lru_list.push_front(pos);
        _lru_map[_cache[pos].block_idx] = _lru_list.begin();
    }

    ssize_t _get_avaiable_pos()
    {
        if (_free_postion.empty())
            return -1;
        ssize_t pos = _free_postion.front();
        _free_postion.pop();
        return pos;
    }

    void _free_lru()
    {
        assert(!_lru_list.empty());
        auto pos = _lru_list.back();
        cache_item &item = _cache[pos];
        _lru_list.pop_back();
        _lru_map.erase(item.block_idx);
        _write_item_back(item);
        item.block_idx = -1;
        _free_postion.push(pos);
    }

    void _get_block_from_disk(size_t block_idx)
    {
        assert(_lru_map.count(block_idx) == 0);
        if (_free_postion.empty())
            _free_lru();
        ssize_t pos = _get_avaiable_pos();
        assert(pos != -1);
        cache_item &item = _cache[pos];
        assert(item.block_idx == -1);
        item.block_idx = block_idx;
        item.dirty = false;
        _disk.read_block(block_idx, item.data);
        _lru_list.push_front(pos);
        _lru_map[block_idx] = _lru_list.begin();
    }

public:
    Cache(Disk &disk, unsigned capacity = 1024) : _disk(disk), _capacity(capacity)
    {
        _cache.resize(capacity);
        for (size_t i = 0; i < capacity; i++)
        {
            _free_postion.push(i);
        }
    };
    ~Cache()
    {
        flush_all();
    }
    void flushb(unsigned block_index)
    {
        auto it = _lru_map.find(block_index);
        assert(it != _lru_map.end());
        auto pos = *(it->second);
        _write_item_back(_cache[pos]);
    }

    void flush_all()
    {
        for (auto &&item : _cache)
        {
            _write_item_back(item);
        }
        _disk.sync();
    }

    void read_block(unsigned block_index, void *buf)
    {
        assert(block_index < DISK_SIZE / BLOCK_SIZE);
        assert(buf != nullptr);
        if (_lru_map.count(block_index) == 0)
            _get_block_from_disk(block_index);
        auto it = _lru_map.find(block_index);
        assert(it != _lru_map.end());
        auto pos = *(it->second);
        memcpy(buf, _cache[pos].data, BLOCK_SIZE);
        _update(it->second);
    }
    void write_block(unsigned block_index, const void *buf)
    {
        assert(block_index < DISK_SIZE / BLOCK_SIZE);
        assert(buf != nullptr);
        if (_lru_map.count(block_index) == 0)
            _get_block_from_disk(block_index);
        auto it = _lru_map.find(block_index);
        assert(it != _lru_map.end());
        auto pos = *(it->second);
        memcpy(_cache[pos].data, buf, BLOCK_SIZE);
        _cache[pos].dirty = true;
        _update(it->second);
    }
};
#endif // __CACHE_H__