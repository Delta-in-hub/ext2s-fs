#ifndef __SHELL_H__
#define __SHELL_H__

#include "vfs.hpp"
#include <mutex>

class Shell
{
    std::mutex &_mtx;
    VFS &_vfs;

    std::string _pwd;

    std::string to_abs(const std::string &dir)
    {
        if (dir.empty())
        {
            return _pwd;
        }
        if (dir[0] == '/')
        {
            return dir;
        }
        if (_pwd.back() != '/')
        {
            return _pwd + '/' + dir;
        }
        return _pwd + dir;
    }

public:
    Shell(VFS &vfs, std::mutex &mtx) : _vfs(vfs), _mtx(mtx)
    {
        _pwd = "/";
    }
    ~Shell()
    {
        _mtx.lock();
        _vfs.sync();
        _mtx.unlock();
    }
    std::string pwd()
    {
        return _pwd;
    }
    std::string cd(const std::string &dir)
    {
        auto abs_dir = to_abs(dir);
        _mtx.lock();
        auto ret = _vfs.exists(abs_dir.c_str());
        _mtx.unlock();
        if (ret == -1)
        {
            return "cd: " + dir + ": No such file or directory";
        }
        else if (ret == 0)
        {
            return "cd: " + dir + ": Not a directory";
        }
        _mtx.lock();
        _pwd = _vfs.real_path(abs_dir.c_str());
        _mtx.unlock();
        return "cd " + dir + ": OK";
    }
    std::string ls(const std::string &dir)
    {
        auto abs_dir = to_abs(dir);
        _mtx.lock();
        auto ret = _vfs.ls(abs_dir.c_str());
        _mtx.unlock();
        if (ret.empty())
        {
            return "ls: " + dir + ": No such file or directory";
        }
        return ret;
    }
    std::string cat(const std::string &file_path)
    {
        auto abs_file = to_abs(file_path);
        char buf[2048];
        memset(buf, 0, sizeof(buf));
        _mtx.lock();
        int fd = _vfs.open(abs_file.c_str(), O_RDONLY);
        if (fd == -1)
        {
            _mtx.unlock();
            return "cat: " + file_path + ": No such file or directory";
        }
        _vfs.read(fd, buf, sizeof(buf));
        _vfs.close(fd);
        _mtx.unlock();
        return buf;
    }

    std::string touch(const std::string &file_path)
    {
        auto abs_file = to_abs(file_path);
        _mtx.lock();
        int ret = _vfs.create(abs_file.c_str());
        if (ret == -1)
        {
            _mtx.unlock();
            return "touch: " + file_path + ": File exists or Path is not valid";
        }
        _mtx.unlock();
        return "touch: " + file_path + ": OK";
    }

    std::string write(const std::string &file_path, const std::string &content, size_t offset)
    {
        auto abs_file = to_abs(file_path);
        _mtx.lock();
        int fd = _vfs.open(abs_file.c_str(), O_WRONLY);
        if (fd == -1)
        {
            _mtx.unlock();
            return "write: " + file_path + ": No such file or directory";
        }
        _vfs.lseek(fd, offset, SEEK_SET);
        _vfs.write(fd, content.c_str(), content.size());
        _vfs.close(fd);
        _mtx.unlock();
        return "write: " + file_path + ": OK";
    }

    std::string unlink(const std::string &file_path)
    {
        auto abs_file = to_abs(file_path);
        _mtx.lock();
        int ret = _vfs.unlink(abs_file.c_str());
        _mtx.unlock();
        if (ret == -1)
        {
            return "unlink: " + file_path + ": No such file or directory or Permission denied";
        }
        return "unlink: " + file_path + ": OK";
    }

    std::string mkdir(const std::string &dir_path)
    {
        auto abs_dir = to_abs(dir_path);
        _mtx.lock();
        int ret = _vfs.mkdir(abs_dir.c_str());
        _mtx.unlock();
        if (ret == -1)
        {
            return "mkdir: " + dir_path + ": Path is not valid";
        }
        return "mkdir: " + dir_path + ": OK";
    }

    std::string rmdir(const std::string &dir_path)
    {
        auto abs_dir = to_abs(dir_path);
        _mtx.lock();
        int ret = _vfs.rmdir(abs_dir.c_str());
        _mtx.unlock();
        if (ret == -1)
        {
            return "rmdir: " + dir_path + ": No such file or directory or not empty";
        }
        return "rmdir: " + dir_path + ": OK";
    }

    std::string mv(const std::string &src, const std::string &dst)
    {
        auto abs_src = to_abs(src);
        auto abs_dst = to_abs(dst);
        std::cout << abs_src << std::endl;
        std::cout << abs_dst << std::endl;
        _mtx.lock();
        int ret = _vfs.mv(abs_src.c_str(), abs_dst.c_str());
        _mtx.unlock();
        if (ret == -1)
        {
            return "mv: " + src + ": No such file or directory";
        }
        return "mv " + src + " " + dst + ": OK";
    }

    std::string real_path(const std::string &path)
    {
        return _vfs.real_path(path.c_str());
    }
};

#endif