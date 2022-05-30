#ifndef __VFS_H__
#define __VFS_H__
#include "ext2m.h"
#include "util.h"

class VFS
{
    Ext2m::Ext2m &_ext2;
    std::string pwd()
    {
        return "";
    }

    int mkdir_from_root(const char *absolute_path)
    {
        assert(absolute_path[0] == '/');
        auto &&paths = split(absolute_path, "/");
        ext2_inode _inode;
        _ext2.get_inode(ROOT_INODE, _inode);
        _inode.
    }

public:
    VFS(Ext2m::Ext2m &ext2) : _ext2(ext2)
    {
        ;
    }
    ~VFS()
    {
        _ext2.sync();
    }
    int open(const char *path, int flags)
    {
        ;
    }
    int close(int fd)
    {
        ;
    }
    ssize_t read(int fd, void *buf, size_t count)
    {
        ;
    }
    ssize_t write(int fd, const void *buf, size_t count)
    {
        ;
    }
    off_t lseek(int fd, off_t offset, int whence)
    {
        ;
    }
    int fstat(int fd, struct stat *buf)
    {
        ;
    }
    int stat(const char *path, struct stat *buf)
    {
        ;
    }
    int mkdir(const char *path)
    {
        auto &&paths = split(path, "/");
        auto dir = pwd();
        if (path[0] == '/') // absolute path
        {
            dir = "/";
        }
    }
    int rmdir(const char *path)
    {
        ;
    }
    int unlink(const char *path)
    {
        ;
    }
    int delet(const char *path)
    {
        unlink(path);
    }
    int create(const char *path)
    {
        ;
    }
    int touch(const char *path)
    {
        return create(path);
    }
    // TODO
    int rename(const char *oldpath, const char *newpath)
    {
        ;
    }
};

#endif