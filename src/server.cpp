#include "extern/Socket.h"
#include "extern/TCPServer.h"
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include "vfs.h"
#include "util.h"
#include "user.h"
#include "shell.h"

using namespace std;

constexpr int COMMAND_LEN = 128;
constexpr int MSG_LEN = 4096;

void debug(const char *msg)
{

    char output[128];
    auto time = std::time(nullptr);
    std::strftime(output, sizeof(output), "%F %T", localtime(&time));
    printf("[%s]: %s\n", output, msg);
}

mutex mtx;
VFS *_vfsp;

void handler(CTCPServer &server, ASocket::Socket socket)
{
    debug("New connection accepted");
    unique_ptr<char> com_buf(new char[COMMAND_LEN]);
    unique_ptr<char> msg_buf(new char[MSG_LEN]);

    auto send_msg = [&](const string &msg)
    {
        memset(msg_buf.get(), 0, MSG_LEN);
        strncpy(msg_buf.get(), msg.c_str(), MSG_LEN);
        server.Send(socket, msg_buf.get(), MSG_LEN);
    };

    User user("userlist.txt");

    int uid = -1;

    // login
    while (true)
    {
        auto len = server.Receive(socket, com_buf.get(), COMMAND_LEN);
        if (len <= 0)
        {
            return;
        }
        debug(com_buf.get());
        auto com = split(com_buf.get(), " ");
        if (com.size() != 3 or com[0] != "login")
        {
            send_msg("Please login first!");
            continue;
        }
        uid = user.login(com[1], com[2]);
        if (uid == -1)
        {
            send_msg("Login failed!");
            continue;
        }
        else
        {
            cout << com[1] << " login success!" << endl;
            send_msg("login_success");
            break;
        }
    }

    Shell sh(ref(*_vfsp), mtx);

    while (true)
    {
        memset(com_buf.get(), 0, COMMAND_LEN);
        auto len = server.Receive(socket, com_buf.get(), COMMAND_LEN);
        if (len <= 0)
        {
            return;
        }
        debug(com_buf.get());
        auto comarr = split(com_buf.get(), " ");
        if (comarr.size() == 0)
        {
            continue;
        }
        auto com = comarr[0];
        if (com == "pwd")
        {
            auto ret = sh.pwd();
            send_msg(ret);
        }
        else if (com == "cd" or com == "chdir")
        {
            if (comarr.size() != 2)
            {
                send_msg("Usage: cd <dir>");
                continue;
            }
            auto ret = sh.cd(comarr[1]);
            send_msg(ret);
        }
        else if (com == "ls" or com == "dir")
        {
            std::string ret;
            if (comarr.size() < 2)
                ret = sh.ls("");
            else
                ret = sh.ls(comarr[1]);
            send_msg(ret);
        }
        else if (com == "cat" or com == "read")
        {
            if (comarr.size() < 2)
            {
                send_msg("cat: missing operand");
                continue;
            }
            auto ret = sh.cat(comarr[1]);
            send_msg(ret);
        }
        else if (com == "mkdir")
        {
            if (comarr.size() < 2)
            {
                send_msg("mkdir: missing operand");
                continue;
            }
            auto ret = sh.mkdir(comarr[1]);
            send_msg(ret);
        }
        else if (com == "rm" or com == "remove")
        {
            if (comarr.size() < 2)
            {
                send_msg("rm: missing operand");
                continue;
            }
            auto ret = sh.unlink(comarr[1]);
            send_msg(ret);
        }
        else if (com == "touch" or com == "create")
        {
            if (comarr.size() < 2)
            {
                send_msg("touch: missing operand");
                continue;
            }
            auto ret = sh.touch(comarr[1]);
            send_msg(ret);
        }
        else if (com == "write")
        {
            if (comarr.size() < 3)
            {
                send_msg("write: missing operand");
                continue;
            }
            auto content = comarr[1];
            auto pos = comarr[2];
            debug(content.c_str());
            debug(pos.c_str());
            size_t offset = 0;
            if (comarr.size() > 3)
            {
                offset = stoi(comarr[3]);
            }
            auto ret = sh.write(pos, content, offset);
            send_msg(ret);
        }
        else if (com == "rmdir")
        {
            if (comarr.size() < 2)
            {
                send_msg("rmdir: missing operand");
                continue;
            }
            auto ret = sh.rmdir(comarr[1]);
            send_msg(ret);
        }
        else if (com == "mv" or com == "rename") // ! bug here
        {
            if (comarr.size() < 3)
            {
                send_msg("mv: missing operand");
                continue;
            }
            auto ret = sh.mv(comarr[1], comarr[2]);
            send_msg(ret);
        }
        else if (com == "exit" or com == "logout")
        {
            debug("Server end close a socket");
            return;
        }
        else
        {
            send_msg("Unknown command!");
        }
    }
}

int main(int argc, char **argv)
{

    printf("Server start\n");

    setbuf(stdout, 0);

    Disk disk("disk.img");
    Cache cache(disk, 8 * BLOCK_SIZE);
    Ext2m::Ext2m ext2fs(cache);
    VFS vfs(ext2fs);
    _vfsp = &vfs;

    std::thread([&]()
                {
        while (true)
        {
            sleep(10);
            mtx.lock();
            vfs.sync();
            mtx.unlock();
        } })
        .detach();

    std::string port = "60000";

    if (argc > 1)
    {
        port = argv[1];
    }

    auto LogPrinter = [](const std::string &strLogMsg)
    { std::cout << strLogMsg << std::endl; };

    CTCPServer TCPServer(LogPrinter, port);

    while (1)
    {
        ASocket::Socket ConnectedClient;
        while (TCPServer.Listen(ConnectedClient))
        {
            std::thread t(handler, std::ref(TCPServer), ConnectedClient);
            t.detach();
        }
    }
}