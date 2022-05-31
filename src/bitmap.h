#ifndef __BITMAP_H__
#define __BITMAP_H__
#include <cassert>
#include <cstdint>
#include <cstring>
#include <tuple>

constexpr unsigned BYTEINBITS = 8;
constexpr uint8_t MSB = 0b10000000; // most significant bit of byte

/**
 * @brief Variable-length bits array.
 * There are a variety of ways to keep track of free record slots on a given page. One efficient method is to use a
 * bitmap: If each data page can hold n records, then you can store an n-bit bitmap in the page header indicating which
 * slots currently contain valid records and which slots are available.
 */
class BitMap
{
private:
    uint8_t *_data;
    unsigned _sizeInBytes;
    unsigned _sizeInBits;
    int _whichByte(int pos) const
    {
        return pos / BYTEINBITS;
    }
    uint8_t _getMask(int pos) const
    {
        return MSB >> (pos % BYTEINBITS);
    }

public:
    BitMap() = delete;
    BitMap(const BitMap &) = delete;
    BitMap(BitMap &&bm)
    {
        _data = bm._data;
        _sizeInBytes = bm._sizeInBytes;
        _sizeInBits = bm._sizeInBits;
        bm._data = nullptr;
        bm._sizeInBits = bm._sizeInBytes = 0;
    }
    /**
     * @brief Construct a new Bit Map object
     *
     * @param data pointer to data
     * @param size length in bits
     */
    BitMap(const void *data, unsigned size) : _sizeInBits(size)
    {
        _sizeInBytes = (_sizeInBits + BYTEINBITS - 1) / BYTEINBITS;
        _data = new uint8_t[_sizeInBytes];
        memcpy(_data, data, _sizeInBytes);
    }
    ~BitMap()
    {
        if (_data)
            delete[] _data;
    }

    /**
     * @brief get the first position of (bit == value) from pos
     *
     * @param pos start position inclusve
     * @return if all bit is 1, return UINTMAX(-1), else return position
     */
    uint32_t nextBit(uint32_t pos = 0, bool value = false) const
    {
        while (pos < _sizeInBits)
        {
            if (get(pos) == value)
                return pos;
            pos++;
        }
        return -1;
    }

    uint32_t count(uint32_t p = 0, bool value = true) const
    {
        uint32_t cnt = 0;
        while (p < _sizeInBits)
        {
            if (get(p) == value)
                cnt++;
            p++;
        }
        return cnt;
    }

    // // useless
    // bool testAll(bool value = false) const
    // {
    //     const uint8_t _value = (value == true) ? 0xff : 0x00;
    //     const uint8_t *_start = _data;
    //     const uint8_t *_end = _data + _sizeInBytes;
    //     while (_start != _end)
    //     {
    //         if (*_start != _value)
    //             return false;
    //         ++_start;
    //     }
    //     return true;
    // }

    /**
     * @brief get a certain bit.
     * @param pos  index from 0
     * @return true for 1b
     * @return false for 0b
     */
    bool get(unsigned pos) const
    {
        assert(pos >= 0 and pos < _sizeInBytes * BYTEINBITS);
        return (_data[_whichByte(pos)] & _getMask(pos)) != 0;
    }

    void set(unsigned pos)
    {
        assert(pos >= 0 and pos < _sizeInBytes * BYTEINBITS);
        _data[_whichByte(pos)] |= _getMask(pos);
    }

    void reset(unsigned pos)
    {
        assert(pos >= 0 and pos < _sizeInBytes * BYTEINBITS);
        _data[_whichByte(pos)] &= (~_getMask(pos));
    }

    void resetAll()
    {
        memset(_data, 0, _sizeInBytes);
    }

    void setAll()
    {
        memset(_data, 0xff, _sizeInBytes);
    }

    unsigned size() const
    {
        return _sizeInBits;
    }
    std::pair<const void *, unsigned> data() const
    {
        return std::make_pair((const void *)(_data), _sizeInBytes);
    }
};
#endif