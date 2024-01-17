// server.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Windows: g++ server.cpp -std=c++11 -o server -lws2_32
// add -lws2_32 flag to link winsocket dependency
// Unix-like: g++ server.cpp -std=c++11 -o server 

// TODO: accept command line argument to set up port number

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "TcpServer.hpp"

class MySever : public EasyTcpServer {
	public:
		// increase number of received message
		virtual void OnNetMsg(ClientSocket* clientSock, DataHeader* header) {
			_msgCount++;
			switch (header->cmd) {
				case CMD_LOGIN: {
					// modify pointer to points to the member of subclass, since the member of parent class
					// would be initialized before those of subclass
					// at the same time, reduce the amount of data we need to read
					Login* login = (Login*)header;
					//std::cout << "Received message from client: " << allCommands[login->cmd] << " message length: " << login->length << std::endl;
					//std::cout << "User: " << login->userName << " Password: " << login->password << std::endl;

					// TODO: when user keep sending message to server, server will crash if it try to response to client
					LoginRet ret;
					// need to decouple to use another thread for sending messages to clients
					clientSock->sendMessage(&ret);
					// TODO: can implement account validation
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

		virtual void OnNetRecv(ClientSocket* clientSock) {
			_recvCount++;
		}

		// new client connect server
		virtual void OnJoin(ClientSocket* clientSock) {
			_clients_list.push_back(clientSock);
			_clientCount++;
		}

		// delete the socket of exited client
		virtual void OnExit(ClientSocket* clientSock) {
			auto iter = _clients_list.end();

			for (int n = (int)_clients_list.size() - 1; n >= 0; n--) {
				if (_clients_list[n] == clientSock) {
					iter = _clients_list.begin() + n;
				}
			}

			if (iter != _clients_list.end()) _clients_list.erase(iter);

			_clientCount--;
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
