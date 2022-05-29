#include "ext2m.h"
#include <bits/stdc++.h>

using namespace std;

int main(void)
{
    Disk disk("disk.img");
    Cache cache(disk, 10240);
    Ext2m::Ext2m ext2fs(cache);
    return 0;
}
