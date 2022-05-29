#ifndef __DISK_H__
#define __DISK_H__
#include <fcntl.h>
#include <error.h>
#include <assert.h>
#include <unistd.h>
#include "ext2_spec.h"
#include "config.h"

class Disk
{
private:
    int _fd;

public:
    Disk(const char *_path)
    {
        // if file exists, open it, otherwise create it
        _fd = open(_path, O_RDWR);
        if (_fd == -1)
        {
            _fd = open(_path, O_RDWR | O_CREAT, 0666);
            assert(_fd != -1);
            auto s = ftruncate(_fd, DISK_SIZE);
            assert(s == 0);
        }
    }
    ~Disk()
    {
        close(_fd);
    }
    void read_block(unsigned block_num, void *buf)
    {
        assert(block_num < DISK_SIZE / BLOCK_SIZE);
        assert(buf != nullptr);
        lseek(_fd, block_num * BLOCK_SIZE, SEEK_SET);
        auto s = read(_fd, buf, BLOCK_SIZE);
        assert(s == BLOCK_SIZE);
    }
    void write_block(unsigned block_num, const void *buf)
    {
        static size_t cnt = 0;
        assert(block_num < DISK_SIZE / BLOCK_SIZE);
        assert(buf != nullptr);
        lseek(_fd, block_num * BLOCK_SIZE, SEEK_SET);
        auto s = write(_fd, buf, BLOCK_SIZE);
        assert(s == BLOCK_SIZE);
        if (cnt++ > 4096)
        {
            cnt = 0;
            s = fsync(_fd); // ! HUGE DAMAGE TO PERFORMANCE !
            assert(s == 0);
        }
    }
};

#endif