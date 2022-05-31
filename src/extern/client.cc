#include "TCPClient.h"
#include "TCPServer.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int main(void)
{
    auto LogPrinter = [](const std::string &strLogMsg)
    { std::cout << strLogMsg << std::endl; };

    CTCPClient TCPClient(LogPrinter);        // creates a TCP client
    TCPClient.Connect("localhost", "12345"); // should return true if the connection succeeds

    int cnt = 20;
    while (cnt--)
    {
        std::string str("Hello World!\n");
        TCPClient.Send(str);
    }
    TCPClient.Disconnect();

    return 0;
}