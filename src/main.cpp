#include "ext2m.h"
#include <bits/stdc++.h>
#include "signal.h"
#include "util.h"
#include "vfs.h"

using namespace std;

int main(void)
{
    Disk disk("disk.img");
    Cache cache(disk, 8 * BLOCK_SIZE);
    Ext2m::Ext2m ext2fs(cache);
    VFS vfs(ext2fs);

    vfs.ls("/");
    vfs.mkdir("/home/delta");
    vfs.touch("/home/delta/Readme.txt");
    vfs.ls("/home/delta");
    vfs.ls("/");
    vfs.ls("/home");

    int fd;
    const char *content = "Hello World!";
    fd = vfs.open("/home/delta/Readme.txt", O_RDWR);
    auto s1 = vfs.write(fd, content, strlen(content) + 1);
    vfs.close(fd);
    fd = vfs.open("/home/delta/Readme.txt", O_RDWR);
    char buf[1024];
    vfs.read(fd, buf, 1024);
    cout << buf << endl;

    return 0;
}
