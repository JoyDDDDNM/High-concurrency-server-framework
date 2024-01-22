// server.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Windows: g++ server.cpp -std=c++11 -o server -lws2_32
// add -lws2_32 flag to link winsocket dependency
// Unix-like: g++ server.cpp -std=c++11 -o server 

// TODO: accept command line argument to set up port number

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Alloc.hpp"
#include "TcpServer.hpp"

class MySever : public EasyTcpServer {
	public:
		// increase number of received message
		virtual void OnNetMsg(ChildServer* pChildServer,ClientSocket* clientSock, DataHeader* header) override {
			EasyTcpServer::OnNetMsg(pChildServer,clientSock, header);

			switch (header->cmd) {
				case CMD_LOGIN: {
					Login* login = (Login*)header;
					//std::cout << "Received message from client: " << allCommands[login->cmd] << " message length: " << login->length << std::endl;
					//std::cout << "User: " << login->userName << " Password: " << login->password << std::endl;

					// TODO: when user keep sending message to server, server will crash if it try to response to client
					LoginRet* ret = new LoginRet();
					pChildServer->addSendTask(clientSock, ret);

					break;
				}
				case CMD_LOGOUT: {
					Logout* logout = (Logout*)header;
					//std::cout << "Received message from client: " << allCommands[logout->cmd] << " message length: " << logout->length << std::endl;
					//std::cout << "User: " << logout->userName << std::endl;
					//// TODO: needs account validation
					//LogoutRet ret;
					//client->sendMessage(&ret);
					break;
				}
				default: {
					std::cout << "Undefined message received from " << clientSock->getSockfd() << std::endl;
					header->length = 0;
					header->cmd = CMD_ERROR;
					clientSock->sendMessage(header);
					break;
				}
			}
		}

		virtual void OnNetRecv(ClientSocket* clientSock) override {
			EasyTcpServer::OnNetRecv(clientSock);
		}

		// new client connect server
		virtual void OnJoin(ClientSocket* clientSock) override {
			EasyTcpServer::OnJoin(clientSock);
		}

		// delete the socket of exited client
		virtual void OnExit(ClientSocket* clientSock) override {
			EasyTcpServer::OnExit(clientSock);
		}
	private:
};

int main() {

	MySever server;

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
    std::cout << "Press Enter any key to exit" << std::endl;
    getchar();
    //Sleep(1000);
    return 0;
}
