#ifndef __VFS_H__
#define __VFS_H__
#include "ext2m.h"
#include "util.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <fcntl.h>

std::string pwd()
{
    return "";
}

class VFS
{
    uint8_t _buf[BLOCK_SIZE];
    Ext2m::Ext2m &_ext2;

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
        _ext2.add_entry_to_inode(inode_idx, e);
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
        _ext2.add_entry_to_inode(inode_idx, e);

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

    int rmdir_from_root(const char *absolute_path)
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
        if (inode_idx == ROOT_INODE)
            return -1;
        auto &&entrys = _ext2.get_inode_all_entry(inode_idx);
        bool delable = true;
        uint32_t father_inode = 0;
        for (auto &&i : entrys)
        {
            if (i.name == ".")
            {
                assert(i.inode == inode_idx);
            }
            else if (i.name == "..")
            {
                father_inode = i.inode;
            }
            else
            {
                delable = false;
                break;
            }
        }
        if (!delable)
            return -1;
        _ext2.free_entry_to_inode(father_inode, inode_idx);
        _ext2.ifree(inode_idx);
        return 0;
    }

    struct file_description
    {
        uint32_t inode_idx;
        uint32_t offset;
        int flag;
    };
    std::vector<file_description> _files;

    int get_avaiable_fd()
    {
        size_t i = 3;
        for (; i < _files.size(); i++)
        {
            if (_files[i].inode_idx == 0)
                return i;
        }
        _files.push_back({0, 0, 0});
        return i;
    }

    bool check_fd(int fd)
    {
        if (fd < 0 || fd >= (int)(_files.size()))
            return false;
        return _files[fd].inode_idx != 0;
    }

    bool check_writeable(int flag)
    {
        if ((flag & O_ACCMODE) == O_RDONLY)
            return false;
        else if ((flag & O_ACCMODE) == O_WRONLY)
            return true;
        else if ((flag & O_ACCMODE) == O_RDWR)
            return true;
        else
            return false;
    }
    bool check_readable(int flag)
    {
        if ((flag & O_ACCMODE) == O_RDONLY)
            return true;
        else if ((flag & O_ACCMODE) == O_WRONLY)
            return false;
        else if ((flag & O_ACCMODE) == O_RDWR)
            return true;
        else
            return false;
    }

    int open_file_from_root(const char *absolute_path, int flag)
    {
        // flag : O_RDONLY, O_WRONLY, O_RDWR
        // check if flag contains O_CREAT

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
        file_description _fdd;
        _fdd.flag = flag;
        _fdd.inode_idx = inode_idx;
        ext2_inode inode;
        _ext2.get_inode(inode_idx, inode);
        if ((inode.i_mode & EXT2_S_IFMT) != EXT2_S_IFREG)
            return -1;
        _fdd.offset = 0;
        auto fd = get_avaiable_fd();
        _files[fd] = _fdd;
        return fd;
    }

public:
    VFS(Ext2m::Ext2m &ext2) : _ext2(ext2)
    {
        // 0,1,2 for stdin,stdout,stderr
        _files.resize(3);
    }
    ~VFS()
    {
        _ext2.sync();
    }
    int open(const char *path, int flags)
    {
        auto str = pwd();
        str += path;
        if ((flags & ~O_ACCMODE) == O_CREAT)
        {
            create(path);
        }
        return open_file_from_root(str.c_str(), flags);
    }
    int close(int fd)
    {
        if (!check_fd(fd))
            return -1;
        _files[fd].inode_idx = 0;
        _files[fd].flag = 0;
        _files[fd].offset = 0;
        return 0;
    }
    ssize_t read(int fd, void *buf, size_t count)
    {
        if (!check_fd(fd))
            return -1;
        if (count == 0)
            return 0;
        auto &&_fd = _files[fd];
        if (not check_readable(_fd.flag))
            return -1;
        ext2_inode inode;
        _ext2.get_inode(_fd.inode_idx, inode);
        inode.i_atime = time(NULL);
        _ext2.write_inode(_fd.inode_idx, inode);
        if (_fd.offset >= inode.i_size)
            return 0;
        size_t real_read_size = 0;
        if (_fd.offset + count > inode.i_size)
            real_read_size = inode.i_size - _fd.offset;
        else
            real_read_size = count;
        auto &&all_blocks = _ext2.get_inode_all_blocks(_fd.inode_idx);

        auto start_block = _fd.offset / BLOCK_SIZE;
        auto start_offset = _fd.offset % BLOCK_SIZE;
        auto end_block = (_fd.offset + real_read_size) / BLOCK_SIZE;
        auto end_offset = (_fd.offset + real_read_size) % BLOCK_SIZE;

        if (start_block == end_block)
        {
            _ext2._disk.read_block(all_blocks[start_block], _buf);
            memcpy(buf, _buf + start_offset, real_read_size);
        }
        else
        {
            for (auto i = start_block; i <= end_block; i++)
            {
                _ext2._disk.read_block(all_blocks[i], _buf);
                if (i == start_block)
                {
                    memcpy(buf, _buf + start_offset, BLOCK_SIZE - start_offset);
                    buf = (uint8_t *)buf + BLOCK_SIZE - start_offset;
                }
                else if (i == end_block)
                {
                    memcpy(buf, _buf, end_offset);
                }
                else
                {
                    memcpy(buf, _buf, BLOCK_SIZE);
                    buf = (uint8_t *)buf + BLOCK_SIZE;
                }
            }
        }

        return real_read_size;
    }
    ssize_t write(int fd, const void *buf, uint32_t count)
    {
        if (!check_fd(fd))
            return -1;
        if (count == 0)
            return 0;
        auto &&_fd = _files[fd];
        if (not check_writeable(_fd.flag))
            return -1;
        auto inode_idx = _fd.inode_idx;
        auto offset = _fd.offset;
        ext2_inode inode;
        _ext2.get_inode(inode_idx, inode);
        inode.i_size = std::max(inode.i_size, offset + count);
        inode.i_atime = time(NULL);
        inode.i_mtime = time(NULL);
        _ext2.write_inode(inode_idx, inode);

        auto &&all_blocks = _ext2.get_inode_all_blocks(inode_idx);

        auto start_block = offset / BLOCK_SIZE;
        auto start_offset = offset % BLOCK_SIZE;
        auto end_block = (offset + count) / BLOCK_SIZE;
        auto end_offset = (offset + count) % BLOCK_SIZE;
        // TODO :SPARSE FILE SUPPORT

        if (end_block >= all_blocks.size())
        {
            int cnt = end_block - all_blocks.size() + 1;
            while (cnt--)
            {
                _ext2.add_block_to_inode(inode_idx);
            }
            all_blocks = _ext2.get_inode_all_blocks(inode_idx);
        }

        if (start_block == end_block)
        {
            _ext2._disk.read_block(all_blocks[start_block], _buf);
            memcpy(_buf + start_offset, buf, count);
            _ext2._disk.write_block(all_blocks[start_block], _buf);
        }
        else
        {
            for (size_t i = start_block; i <= end_block; i++)
            {
                auto &&block = all_blocks[i];

                if (i == start_block)
                {
                    _ext2._disk.read_block(block, _buf);
                    memcpy(_buf + start_offset, buf, BLOCK_SIZE - start_offset);
                    _ext2._disk.write_block(block, _buf);
                    buf = (uint8_t *)(buf) + BLOCK_SIZE - start_offset;
                }
                else if (i == end_block)
                {
                    _ext2._disk.read_block(block, _buf);
                    memcpy(_buf, buf, end_offset);
                    _ext2._disk.write_block(block, _buf);
                }
                else
                {
                    _ext2._disk.read_block(block, _buf);
                    memcpy(_buf, buf, BLOCK_SIZE);
                    _ext2._disk.write_block(block, _buf);
                    buf = (uint8_t *)(buf) + BLOCK_SIZE;
                }
            }
        }
        _fd.offset += count;
        return count;
    }
    off_t lseek(int fd, off_t offset, int whence)
    {
        if (!check_fd(fd))
            return -1;
        ext2_inode inode;
        auto inode_idx = _files[fd].inode_idx;
        _ext2.get_inode(inode_idx, inode);
        switch (whence)
        {
        case SEEK_SET:
            _files[fd].offset = offset;
            break;
        case SEEK_CUR:
            _files[fd].offset += offset;
            break;
        case SEEK_END:
            _files[fd].offset = inode.i_size + offset;
            break;
        default:
            return -1;
        }
        return _files[fd].offset;
    }
    int fstat(int fd, struct stat *buf)
    {
        if (!check_fd(fd))
            return -1;
        ext2_inode inode;
        auto inode_idx = _files[fd].inode_idx;
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
        auto dir = pwd();
        dir += path;
        return rmdir_from_root(dir.c_str());
        return -1;
    }
    int unlink(const char *path)
    {
        return -1;
    }
    int delet(const char *path)
    {
        unlink(path);
        return -1;
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
        return -1;
    }
};

#endif