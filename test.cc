#include <fcntl.h>
#include <string>
#include <cstdio>
#include <iostream>
#include <unistd.h>

#include "./src/cache.h"

using namespace std;

int main(int argc, char **argv)
{
    Disk disk("disk.img");
    Cache cache(disk, 2);
    void *buf = new uint8_t[BLOCK_SIZE];
    cache.read_block(10, buf);
    cache.read_block(11, buf);
    cache.read_block(12, buf);
    cache.read_block(13, buf);
    cache.write_block(14, buf);
    cache.write_block(10, buf);
    return 0;
}