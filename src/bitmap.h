#ifndef __BITMAP_H__
#define __BITMAP_H__
#include <cassert>
#include <cstdint>
#include <cstring>

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
    /**
     * @brief Construct a new Bit Map object
     *
     * @param data pointer to data
     * @param size length in bytes
     */
    BitMap(void *data, unsigned size) : _data(static_cast<uint8_t *>(data)), _sizeInBytes(size)
    {
        ;
    }

    /**
     * @brief get the first position of (bit == value) from pos
     *
     * @param pos start position inclusve
     * @return if all bit is 1, return UINTMAX(-1), else return position
     */
    uint32_t nextBit(uint32_t pos = 0, bool value = false)
    {
        while (pos < _sizeInBytes * BYTEINBITS)
        {
            if (get(pos) == value)
                return pos;
            pos++;
        }
        return -1;
    }

    uint32_t count(uint32_t p = 0, bool value = true)
    {

        uint32_t cnt = 0;
        while (p < _sizeInBytes * BYTEINBITS)
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
    bool get(int pos) const
    {
        assert(pos >= 0 and pos < _sizeInBytes * BYTEINBITS);
        return (_data[_whichByte(pos)] & _getMask(pos)) != 0;
    }

    void set(int pos)
    {
        assert(pos >= 0 and pos < _sizeInBytes * BYTEINBITS);
        _data[_whichByte(pos)] |= _getMask(pos);
    }

    void reset(int pos)
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

    /**
     * @brief Get the Length of bitmap in bits
     * @return bits length
     */
    unsigned getLength() const
    {
        return _sizeInBytes * BYTEINBITS;
    }
};
#endif