#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN	// macro to avoid including duplicate macro when include <windows.h> and <WinSock2.h>
#	include <windows.h>			// windows system api
#	include <WinSock2.h>		// windows socket api 
#else
#	include <unistd.h>			// unix standard symbolic constants and types
#	include <arpa/inet.h>		//definitions for internet operations
#	include <string>
#	include <string.h>
#	define SOCKET int
#	define INVALID_SOCKET  (SOCKET)(~0)
#	define SOCKET_ERROR            (-1)
#endif

#ifndef RECV_BUFF_SIZE 
// base unit of buffer size
#define RECV_BUFF_SIZE 10240
#endif

#include <iostream>
#include <vector>
#include <thread>
#include "Message.hpp"

class EasyTcpClient
{
public:
	EasyTcpClient() :_sock{ INVALID_SOCKET }, _szRecv{ {} }, _szMsgBuf{ {} }, _offset{0} {}

	// initialize socket of client to connect server
	int initSocket() {
		// lanuch Win Sock 2.x environment
#		ifdef _WIN32
			// launch windows socket 2.x environment
			WORD ver = MAKEWORD(2, 2);
			WSADATA dat;

			// initiates use of the Winsock DLL by program.
			WSAStartup(ver, &dat);
#		endif
		// socket has been created, close it and create an new one
		if (INVALID_SOCKET != _sock) {
			std::cout << "socket: " << _sock << " has been created previously, close it and recreate an new socket" << std::endl;
			closeSock();
		}
		
		// 1. build socket
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			std::cout << "socket create failed" << std::endl;
			return -1;
		}

		return 0;
	}

	// connect to server with sepcfied ip and port number
	int connectServer(const char* ip,unsigned short port) {

		if (INVALID_SOCKET == _sock) {
			std::cout << "socket did not created, create socket automatically for connecting server" << std::endl;
			initSocket();
		}

		// 2. connect server
		// https://stackoverflow.com/questions/21099041/why-do-we-cast-sockaddr-in-to-sockaddr-when-calling-bind
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);

#		ifdef _WIN32
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#		else
			_sin.sin_addr.s_addr = inet_addr(ip);
#		endif

		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));

		if (SOCKET_ERROR == ret) {
			std::cout << "Server connect failed" << std::endl;
		}
		else {
			std::cout << "Server connect succeed" << std::endl;
		}

		return ret;
	}

	// close client socket
	void closeSock() {
		if (INVALID_SOCKET != _sock) {
#			ifdef _WIN32
				closesocket(_sock);
				WSACleanup();
#			else
				close(_sock);
#			endif
				_sock = INVALID_SOCKET;
		}
	}

	// listen response from server
	bool listenServer() {
		// socket is not created
		if (!isRun()) {
			return false;
		}

		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);

		timeval t = { 1, 0 };

		// client is blocked since the socket of server side is closed
		int ret = select(_sock + 1, &fdRead, 0, 0, &t);
		if (ret < 0) {
			std::cout << "server is closed " << std::endl;
			closeSock();
			return false;
		}

		if (FD_ISSET(_sock, &fdRead)) {
			FD_CLR(_sock, &fdRead);

			if (receiveServerMessage(_sock) == -1) {
				closeSock();
				return false;
			};
		}

		return true;
	}

	// receive message from server, solve message concatenation
	int receiveServerMessage(SOCKET _cSock) {
		// 5. keeping reading message from server
		//we only read header info from the incoming message
		int nLen = (int)recv(_cSock, _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0) {
			// connection has closed
			return -1;
		}

		// copy all messages from the received buffer to second buffer 
		memcpy(_szMsgBuf + _offset, _szRecv, nLen);

		// increase offset so that the next message will be moved to the end of the previous message
		_offset += nLen;

		// receive at least one full dataheader, 
		// repeatedly process the incoming message, which solve packet concatenation
		
		while (_offset >= sizeof(DataHeader)) {
			DataHeader* header = (DataHeader*)_szMsgBuf;

			// receive a full message including data header
			if (_offset >= header->length) {

				// the length of all following messages
				int shiftLen = _offset - header->length;

				// we dont need to receive the message body since 
				processServerMessage(header);

				// successfully processs the first message
				// shift all following messages to the beginning of second buffer
				memcpy(_szMsgBuf, _szMsgBuf + header->length, shiftLen);

				_offset = shiftLen;
				
			}
			else {
				// the remaining message is not complete, wait for socket to receive the
				// them until we get a full next message
				break;
			}
		}

		return 0;
	}

	// check if socket is created
	bool isRun() {
		return _sock != INVALID_SOCKET;
	}

	// prompt message to client for the received message from server
	void processServerMessage(DataHeader* header) {
		switch (header->cmd) {
			case CMD_LOGIN_RESULT: {
				LoginRet* loginRet = (LoginRet*)header;
				std::cout << "Receive message from server: login successfully" << std::endl;
				std::cout << "Message length: " << loginRet->length << std::endl;
				break;
			}
			case CMD_LOGOUT_RESULT: {
				LogoutRet* logoutRet = (LogoutRet*)header;
				std::cout << "Receive message from server: logout successfully" << std::endl;
				std::cout << "Message length: " << logoutRet->length << std::endl;
				break;
			}
			case CMD_NEW_USER_JOIN: {
				NewUserJoin* newUser = (NewUserJoin*)header;
				std::cout << "Receive message from server: new user join into server" << std::endl;
				std::cout << "user id: " << newUser->cSocket << std::endl;
				std::cout << "Message length: " << newUser->length << std::endl;
				break;
			}
			case CMD_ERROR: {
				NewUserJoin* newUser = (NewUserJoin*)header;
				std::cout << "Receive message from server: new user join into server" << std::endl;
				std::cout << "user id: " << newUser->cSocket << std::endl;
				std::cout << "Message length: " << newUser->length << std::endl;
				std::cout << "Error command received" << std::endl;
				break;
			}
			default: {
				std::cout << "Undefined message" << std::endl;
				break;
			}
		}
	}

	// when user type message, send message to server
	int sendMessage(DataHeader* header) {
		int ret = SOCKET_ERROR;
		
		if (isRun() && header) {
			ret = send(_sock, (const char*)header, header -> length, 0);

			// server is close, needs to close client socket
			if (ret == SOCKET_ERROR) closeSock();
		}

		return ret;
	}

	virtual ~EasyTcpClient() {
		closeSock();
	}

private:
	SOCKET _sock;

	// buffer for receiving data, this is still a fixed length buffer
	char _szRecv[RECV_BUFF_SIZE];

	// second buffer to store data after we receive it from the buffer inside the OS
	char _szMsgBuf[RECV_BUFF_SIZE * 10];

	// offset pointer which points to the end a sequence of messages received from _szRecv
	int _offset;
};

bool isRun = true;
void cmdThread() {
//void cmdThread(EasyTcpClient* client) {
	while (isRun) {
		char cmdBuf[256] = {};
		std::cin >> cmdBuf;
		if (strcmp(cmdBuf, "exit") == 0) {
			std::cout << "sub thread finished" << std::endl;
			// tell main thread that the sub thread is finished
			isRun = false;
			//client->closeSock();
			break;
		}
		else if (strcmp(cmdBuf, "login") == 0) {
			Login login;
			strcpy(login.userName, "account");
			strcpy(login.password, "password");
			//client->sendMessage(&login);
		}
		else if (strcmp(cmdBuf, "logout") == 0) {
			Logout logout;
			strcpy(logout.userName, "account");
			//client->sendMessage(&logout);
		}
		else {
			std::cout << "not valid command" << std::endl;
		}
	}
}

#endif