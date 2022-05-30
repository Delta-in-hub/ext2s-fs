#ifndef __VFS_H__
#define __VFS_H__
#include "ext2m.h"
#include "util.h"
#include <algorithm>
#include <cstdio>
#include <iostream>

class VFS
{
    uint8_t _buf[BLOCK_SIZE];
    Ext2m::Ext2m &_ext2;
    std::string pwd()
    {
        return "";
    }

    ssize_t find_dir_from_inode(uint32_t inode_idx, const std::string &name, bool creat = false)
    {
        auto entrys = _ext2.get_inode_all_entry(inode_idx);
        for (auto &&j : entrys)
        {
            if (j.name == name)
            {
                return j.inode;
            }
        }
        if (not creat)
            return -1;
        auto newid = _ext2.ialloc();
        if (newid == 0)
            return -1;
        size_t group = newid / _ext2.inodes_per_group;
        ext2_inode inode;
        // TODO: UID, GID, mode
        _ext2.init_inode(inode, EXT2_S_IFDIR | 0755, 0, 0);
        inode.i_block[0] = _ext2.ballocs(group, 1).front();
        _ext2.write_inode(newid, inode);

        memset(_buf, 0, BLOCK_SIZE);
        _ext2.init_entry_block(_buf, newid, inode_idx);
        _ext2._disk.write_block(inode.i_block[0], _buf);

        Ext2m::Ext2m::entry e;
        e.file_type = EXT2_FT_DIR;
        e.inode = newid;
        e.name = name;
        _ext2.add_entry_to_indoe(inode_idx, e);
        return newid;
    }

    int mkdir_from_root(const char *absolute_path)
    {
        assert(absolute_path[0] == '/');
        auto &&paths = split(absolute_path, "/");
        uint32_t inode_idx = ROOT_INODE;
        for (auto &&i : paths)
        {
            auto idx = find_dir_from_inode(inode_idx, i, true);
            if (idx == -1)
                return -1;
            inode_idx = idx;
        }
        return 0;
    }

    int create_file_from_root(const char *absolute_path)
    {
        assert(absolute_path[0] == '/');
        auto &&paths = split(absolute_path, "/");
        auto file_name = paths.back();
        paths.pop_back();
        if (file_name.size() > EXT2_NAME_LEN)
            return -1;
        uint32_t inode_idx = ROOT_INODE;
        for (auto &&i : paths)
        {
            auto idx = find_dir_from_inode(inode_idx, i);
            if (idx == -1)
                return -1;
            inode_idx = idx;
        }
        auto idx = find_dir_from_inode(inode_idx, file_name);
        if (idx != -1)
            return -1;
        auto nid = _ext2.ialloc();
        if (nid == 0)
            return -1;
        ext2_inode inode;
        _ext2.init_inode(inode, EXT2_S_IFREG | 0644, 0, 0);
        _ext2.write_inode(nid, inode);
        Ext2m::Ext2m::entry e;
        e.file_type = EXT2_FT_REG_FILE;
        e.inode = nid;
        e.name = file_name;
        _ext2.add_entry_to_indoe(inode_idx, e);

        return 0;
    }

    int stat_from_root(const char *absolute_path, struct stat *buf)
    {
        assert(absolute_path[0] == '/');
        auto &&paths = split(absolute_path, "/");
        uint32_t inode_idx = ROOT_INODE;
        for (auto &&i : paths)
        {
            auto idx = find_dir_from_inode(inode_idx, i);
            if (idx == -1)
                return -1;
            inode_idx = idx;
        }
        ext2_inode inode;
        _ext2.get_inode(inode_idx, inode);
        buf->st_ino = inode_idx;
        buf->st_mode = inode.i_mode;
        buf->st_size = inode.i_size;
        buf->st_nlink = inode.i_links_count;
        buf->st_uid = inode.i_uid;
        buf->st_gid = inode.i_gid;
        buf->st_atime = inode.i_atime;
        buf->st_mtime = inode.i_mtime;
        buf->st_ctime = inode.i_ctime;
        return 0;
    }

    int ls_from_root(const char *absolute_path)
    {
        assert(absolute_path[0] == '/');
        auto &&paths = split(absolute_path, "/");
        uint32_t inode_idx = ROOT_INODE;
        for (auto &&i : paths)
        {
            auto idx = find_dir_from_inode(inode_idx, i);
            if (idx == -1)
                return -1;
            inode_idx = idx;
        }
        auto entrys = _ext2.get_inode_all_entry(inode_idx);
        for (auto &&i : entrys)
        {
            printf("%s\t%d\t%d\n", i.name.c_str(), i.file_type, i.inode);
        }
        return 0;
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
        auto dir = pwd();
        dir += path;
        return stat_from_root(dir.c_str(), buf);
    }
    int mkdir(const char *path)
    {
        auto dir = pwd();
        dir += path;
        return mkdir_from_root(dir.c_str());
    }

    int ls(const char *path)
    {
        auto dir = pwd();
        dir += path;
        return ls_from_root(dir.c_str());
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
        auto dir = pwd();
        dir += path;
        return create_file_from_root(dir.c_str());
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