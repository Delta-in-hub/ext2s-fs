#include <iostream>
#include <string>
#include "extern/Socket.h"
#include "extern/TCPClient.h"

using namespace std;

constexpr int COMMAND_LEN = 128;
char send_buf[COMMAND_LEN * 10];

constexpr int MSG_LEN = 4096;
char ret_buf[MSG_LEN];

int main(int argc, char** argv) {
    //设置流缓冲区
    setbuf(stdout, 0);
    std::string ipaddress = "localhost";
    std::string port = "60000";

    if (argc > 1) {
        ipaddress = argv[1];
    } else if (argc > 2) {
        port = argv[2];
    }

    auto LogPrinter = [](const std::string& strLogMsg) {};
    // { std::cout << strLogMsg << std::endl; };

    CTCPClient client(LogPrinter);  // creates a TCP client
    auto flag = client.Connect(
        ipaddress, port);  // should return true if the connection succeeds

    if (not flag) {
        printf("Connect to the srever(%s:%s) failed!\n", ipaddress.c_str(),
               port.c_str());
        return -1;
    }

    auto send_command = [&](const string& com) {
        if (com.size() >= COMMAND_LEN) {
            printf("command is too long!\n");
            return;
        }
        memset(send_buf, 0, sizeof(send_buf));//Memory initialization
        strcpy((char*)send_buf, com.c_str());
        client.Send(send_buf, COMMAND_LEN);
    };

    // Welcome to Ubuntu 20.04.4 LTS (GNU/Linux 5.4.0-110-generic x86_64)

    printf("Connect to the srever(%s:%s) success!\n", ipaddress.c_str(),
           port.c_str());
    // printf("Welcome to EXT2 File System Client End!\n\n");

    while (true) {
        printf("Please input your username: ");
        std::string username;
        std::cin >> username;
        printf("Please input your password: ");
        std::string password;
        std::cin >> password;
        printf("\n");
        send_command("login " + username + " " + password);
        memset(ret_buf, 0, sizeof(ret_buf));
        client.Receive(ret_buf, MSG_LEN);
        if (strncmp(ret_buf, "login_success", strlen("login_success")) == 0) {
            printf("Login success!\n");
            printf("Welcome %s to EXT2 File System Client End!\n\n",
                   username.c_str());
            break;
        } else {
            printf("Login failed!\n");
        }
    }

    while (true) {
        printf("> ");
        std::string command;
        getline(std::cin, command);
        if (command.empty())
            continue;
        send_command(command);
        if (command == "exit" or command == "logout")
            break;
        memset(ret_buf, 0, sizeof(ret_buf));
        client.Receive(ret_buf, MSG_LEN);
        printf("%s\n", ret_buf);
    }

    client.Disconnect();
    return 0;
}
