#include "TCPClient.h"
#include "TCPServer.h"
#include <iostream>
#include <unistd.h>
#include <thread>

using namespace std;

int main(void)
{
    setbuf(stdout, 0);
    auto LogPrinter = [](const std::string &strLogMsg)
    { std::cout << strLogMsg << std::endl; };

    CTCPServer TCPServer(LogPrinter, "12345"); // creates a TCP server to listen on port 12345

    while (1)
    {
        ASocket::Socket ConnectedClient; // socket of the connected client, we can have a vector of them for example.
        /* blocking call, should return true if the accept is OK, ConnectedClient should also be a valid socket
        number */
        while (TCPServer.Listen(ConnectedClient))
        {
            std::thread([&](ASocket::Socket client)
                        {
                char buf[1024];
                while (TCPServer.Receive(client, buf, sizeof(buf)))
                {
                    cout << buf << endl;
                } 
                cout << this_thread::get_id() << " disconnected" << endl; },
                        ConnectedClient)
                .detach();
        }
    }

    return 0;
}