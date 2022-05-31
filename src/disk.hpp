#ifndef __DISK_H__
#define __DISK_H__
#include <fcntl.h>
#include <error.h>
#include <assert.h>
#include <unistd.h>
#include "ext2_spec.h"
#include "config.hpp"
#include <cstdlib>
#include <cstdio>

class Disk
{
private:
    FILE *_fp;

public:
    Disk(const char *_path)
    {
        // if file exists, open it, otherwise create it
        _fp = fopen(_path, "r");
        if (_fp == nullptr)
        {
            _fp = fopen(_path, "w+b");
            assert(_fp != nullptr);
            fseek(_fp, DISK_SIZE, SEEK_END);
            fwrite("\0", 1, 1, _fp);
        }
        else
        {
            fclose(_fp);
            _fp = fopen(_path, "r+b");
        }
        assert(_fp);
    }
    ~Disk()
    {
        fclose(_fp);
    }
    void read_block(unsigned block_num, void *buf)
    {
        assert(block_num < DISK_SIZE / BLOCK_SIZE);
        assert(buf != nullptr);
        fseek(_fp, block_num * BLOCK_SIZE, SEEK_SET);
        auto s = fread(buf, 1, BLOCK_SIZE, _fp);
        assert(s == BLOCK_SIZE);
    }
    void write_block(unsigned block_num, const void *buf)
    {
        static size_t cnt = 0;
        assert(block_num < DISK_SIZE / BLOCK_SIZE);
        assert(buf != nullptr);
        fseek(_fp, block_num * BLOCK_SIZE, SEEK_SET);
        auto s = fwrite(buf, 1, BLOCK_SIZE, _fp);
        assert(s == BLOCK_SIZE);
        if (cnt++ > 4096)
        {
            cnt = 0;
            // s = fsync(_fd); // ! HUGE DAMAGE TO PERFORMANCE !
            s = fflush(_fp);
            assert(s == 0);
        }
    }
    void sync()
    {
        // auto s = fsync(_fd);
        auto s = fflush(_fp);
        assert(s == 0);
    }
};

#endif