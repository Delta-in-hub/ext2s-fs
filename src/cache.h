#ifndef __CACHE_H__
#define __CACHE_H__
#include "disk.h"
#include <unordered_map>
#include <list>
#include <string.h>
// LRU CACHE FOR DISK
class Cache
{
private:
    Disk &_disk;
    const unsigned _capacity; // LRU CACHE CAPACITY

    uint8_t *_data;
    // dirty flag
    __uint8_t *_dirty;

    // _array + BLOCK_SIZE * n(position in _array)
    std::list<unsigned> _positon;
    // block_num to position in _array
    std::unordered_map<unsigned, std::list<unsigned>::iterator> _b_to_p;
    // position to block_num
    std::unordered_map<unsigned, unsigned> _p_to_b;
    void flushp(unsigned position)
    {
        auto it = _p_to_b.find(position);
        assert(it != _p_to_b.end());
        auto block_num = it->second;
        if (_dirty[position])
        {
            _disk.write_block(block_num, _data + position);
            _dirty[position] = 0;
        }
    }
    void pop_cache()
    {
        assert(_positon.size() > 0);
        auto p = _positon.back();
        if (_dirty[p])
        {
            flushp(p);
        }
        _positon.pop_back();
        auto bp = _p_to_b.find(p);
        assert(bp != _p_to_b.end());
        auto b = bp->second;
        _p_to_b.erase(p);
        _b_to_p.erase(b);
    }
    auto get_block_to_cache(unsigned block_num)
    {
        if (_positon.size() >= _capacity)
        {
            pop_cache();
        }
        assert(_positon.size() < _capacity);
        auto p = get_avaiable_position();
        assert(p != -1);
        _disk.read_block(block_num, _data + p * BLOCK_SIZE);
        _positon.push_front(p);
        _b_to_p[block_num] = _positon.begin();
        _p_to_b[p] = block_num;
        _dirty[p] = 0;
        return p;
    }
    int get_avaiable_position()
    {
        for (int i = 0; i < _capacity; i++)
        {
            if (_p_to_b.count(i) == 0)
            {
                assert(_dirty[i] == 0);
                return i;
            }
        }
        return -1;
    }

public:
    Cache(Disk &disk, unsigned capacity = 1024) : _disk(disk), _capacity(capacity)
    {
        _data = new uint8_t[BLOCK_SIZE * _capacity];
        memset(_data, 0, BLOCK_SIZE * _capacity);
        _dirty = new uint8_t[_capacity];
        memset(_dirty, 0, _capacity);
    };
    ~Cache()
    {
        flush_all();
        delete[] _data;
        delete[] _dirty;
    }
    void flushb(unsigned block_num)
    {
        auto it = _b_to_p.find(block_num);
        assert(it != _b_to_p.end());
        auto p = *(it->second);
        if (_dirty[p])
        {
            _disk.write_block(block_num, _data + p * BLOCK_SIZE);
            _dirty[p] = 0;
        }
    }

    void flush_all()
    {
        for (auto it = _positon.begin(); it != _positon.end(); it++)
        {
            flushp(*it);
        }
    }

    void read_block(unsigned block_num, void *buf)
    {
        assert(block_num < DISK_SIZE / BLOCK_SIZE);
        assert(buf != nullptr);
        void *ptr = nullptr;
        auto it = _b_to_p.find(block_num);
        auto p = 0ul;
        if (it == _b_to_p.end())
            p = get_block_to_cache(block_num);
        else
            p = *(it->second);
        ptr = _data + BLOCK_SIZE * p;
        memcpy(buf, ptr, BLOCK_SIZE);
    }
    void write_block(unsigned block_num, const void *buf)
    {
        assert(block_num < DISK_SIZE / BLOCK_SIZE);
        assert(buf != nullptr);
        void *ptr = nullptr;
        auto it = _b_to_p.find(block_num);
        auto p = 0ul;
        if (it == _b_to_p.end())
            p = get_block_to_cache(block_num);
        else
            p = *(it->second);
        ptr = _data + BLOCK_SIZE * p;
        _dirty[p] = 1;
        memcpy(ptr, buf, BLOCK_SIZE);
    }
};
#endif // __CACHE_H__