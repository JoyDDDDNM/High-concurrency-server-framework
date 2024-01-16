// server.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Windows: g++ server.cpp -std=c++11 -o server -lws2_32
// add -lws2_32 flag to link winsocket dependency
// Unix-like: g++ server.cpp -std=c++11 -o server 

// TODO: accept command line argument to set up port number

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "EasyTcpServer.hpp"

int main() {

    EasyTcpServer server;

    server.initSocket();

    server.bindPort(nullptr,4567);

    server.listenNumber(5);

    server.Start(4);

    // create an thread for reading server input
    std::thread serverCmdThread(cmdThread);
    serverCmdThread.detach();

    while (isRun) {
        server.onRun();
    }

    server.closeSock();

    std::cout << "server closed" << std::endl;
    //Sleep(1000);
    return 0;
}
