# ext2s-fs

> https://github.com/Delta-in-hub/ext2s-fs

EXT2-Simplified File System Implementation (ext2s-fs).
Cross-platform , based on C++14.
Support multiple users(clients).
The struct definition is same to the real ext2 specification.
Works for Block-Size = 1024Byte.

## Build

```bash
> git clone https://github.com/Delta-in-hub/ext2s-fs.git
> cd ext2s-fs
> make all # on Windows>  mingw32-make.exe all
> cd bin
> ./server # server end
> ./client # client end
```



## Design

![ext2-vfs](http://www.meteck.org/IMAGES/ext2-vfs.gif)

- `config.h`: Configuration of my ext2s-fs.
- `ext2_spec.h`: ext2 specification
- `bitmap.hpp`: Bitmap class
- `util.hpp`: Utility functions
- `disk.hpp`: Disk interface. Read or Write with block size = 1024Byte.
- `cache.hpp`: LRU Cache. Cache the disk block data.
- `ext2m.hpp`: ext2s implementation. Manage the block, inode, entry.
- `vfs.hpp`: Virtual File System. Provide the api like `open` `read` `write` etc..
- `shell.hpp`: Command line tools like `cat` `touch` ...
- `user.hpp`: User management. `userlist` is in `bin/userlist.txt`.
  - `uid  usrename  password`

- `server.cpp`: Server end for ext2s-fs, listen on port 60000(default).
- `client.cpp`: Client end for ext2s-fs, connect to server and provide the terminal interface.