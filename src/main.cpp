#include "ext2m.h"
#include <bits/stdc++.h>
#include "signal.h"
#include "util.h"
#include "vfs.h"
#include "shell.h"

using namespace std;

int main(void)
{
    Disk disk("disk.img");
    Cache cache(disk, 8 * BLOCK_SIZE);
    Ext2m::Ext2m ext2fs(cache);
    VFS vfs(ext2fs);

    cout << vfs.ls("/") << endl;
    vfs.mkdir("/home");
    vfs.mkdir("/home/delta");
    cout << vfs.ls("/") << endl;
    cout << vfs.ls("/home") << endl;

    vfs.create("/home/delta/Readme.txt");
    int fd = vfs.open("/home/delta/Readme.txt", O_WRONLY);
    vfs.write(fd, "Hello World", 11);
    vfs.close(fd);
    cout << vfs.ls("/home/delta") << endl;

    char buf[512];
    memset(buf, 0, sizeof(buf));
    fd = vfs.open("/home/delta/Readme.txt", O_RDONLY);
    vfs.read(fd, buf, 512);
    cout << buf << endl;
    vfs.close(fd);

    return 0;
}
