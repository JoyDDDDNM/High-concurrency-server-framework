#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _WIN32				
// in Unix-like environment, the maximum size of fd_set is 1024
// in windows environment, the maximum size of fd_set is 64, in WinSock2.h
// to ensure the consitentcy for cross-platform development, 
// we can increase its size by refining this macro 
#	define FD_SETSIZE 1024 
#   define WIN32_LEAN_AND_MEAN // macro to avoid including duplicate macro when include <windows.h> and <WinSock2.h>
#include <windows.h>  // windows system api
#include <WinSock2.h> // windows socket api 
#else
#   include <unistd.h> // unix standard system interface
#   include <arpa/inet.h>
#	include <string.h>
#   define SOCKET int
#   define INVALID_SOCKET  (SOCKET)(~0)
#   define SOCKET_ERROR            (-1)
#endif

#ifndef RECV_BUFF_SIZE 
// base unit of buffer size
#define RECV_BUFF_SIZE 10240
#endif

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>
#include <map>
#include "Message.hpp"
#include "CELLTimestamp.hpp"

std::vector<std::string> allCommands = { "CMD_LOGIN",
										"CMD_LOGIN_RESULT",
										"CMD_LOGOUT",
										"CMD_LOGOUT_RESULT",
										"CMD_NEW_USER_JOIN",
										"CMD_ERROR" };
										
class ClientSocket {
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET) :_sockfd{ sockfd }, _szMsgBuf{ {} }, _offset{ 0 } {}

	SOCKET getSockfd() {
		return _sockfd;
	}

	char* getMsgBuf() {
		return _szMsgBuf;
	}

	int getOffset() {
		return _offset;
	}

	void setOffset(int pos) {
		_offset = pos;
	} 

	// send message to client
	int sendMessage(DataHeader* header) {
		if (header) {
			return send(_sockfd, (const char*)header, header->length, 0);
		}

		return SOCKET_ERROR;
	}

private:
	// socket fd, which will be put into selcet function
	SOCKET _sockfd;

	// second buffer to store data after we receive it from the buffer inside the OS
	char _szMsgBuf[RECV_BUFF_SIZE * 5];

	// offset pointer which points to the end a sequence of messages received from _szRecv
	int _offset;

};

class INetEvent
{
public:
	INetEvent() = default;

	// client exits server
	virtual void OnJoin(ClientSocket* clientSock) = 0;
	virtual void OnExit(ClientSocket* clientSock) = 0;
	virtual void OnNetMsg(ClientSocket* clientSock, DataHeader* header) = 0;
	virtual void OnNetRecv(ClientSocket* clientSock) = 0;
	~INetEvent() = default;

private:

};

class ChildServer {
	public:
		ChildServer(SOCKET sock = INVALID_SOCKET) :_sock{ sock }, 
												_clients{},
												_clients_Buffer{},
												_mutex{},
												_pThread{}, 
												_netEvent{ nullptr }
		{}

		// check if socket is creaBted
		bool isRun() {
			return _sock != INVALID_SOCKET;
		}

		// close socket
		void closeSock() {
			if (_sock == INVALID_SOCKET) {
				return;
			}

#		ifdef _WIN32
			// close all client sockets
			for (auto iter : _clients) {
				closesocket(iter.second->getSockfd());
				// TODO: need to check if it is not nullptr and delete successfully
				delete iter.second;
			}
			// terminates use of the Winsock 2 DLL (Ws2_32.dll)
			closesocket(_sock);
			WSACleanup();
#		else
			for (auto iter : _clients) {
				close(iter.second->getSockfd());
				delete iter.second;
			}
			close(_sock);
#		endif
			_clients.clear();
		}

		// backup previous file descriptor set to improve performance
		fd_set _fdRead_pre;
		bool _clients_change;

		// used in linux environment
		SOCKET _maxSock;

		// keep running to listen client message
		void listenClient() {
			_clients_change = true;
			while (isRun()) {
				// check if buffer queue contain any connected clients
				if (_clients_Buffer.size() > 0) {
					// lock guard will release lock automatically when reach the end of scope to deconstruct itself
					std::lock_guard<std::mutex> _lock(_mutex);

					for (auto client : _clients_Buffer) {
						_clients[client->getSockfd()] = client;
					}
				
					_clients_Buffer.clear();
					_clients_change = true;
				}

				if (_clients.empty()) {
					// when no client connected, sleep child server
					std::chrono::milliseconds t(1);
					std::this_thread::sleep_for(t);
					continue;
				}

				// fd_set: a struct which can be placed sockets into a "set" for various purposes, such as testing a given socket for readability using the readfds parameter of the select function
				fd_set fdRead;
				//fd_set fdWrite;
				//fd_set fdExp;

				// reset the count of each set to zero
				FD_ZERO(&fdRead);
				//FD_ZERO(&fdWrite);
				//FD_ZERO(&fdExp);

				// only update file descriptor set when client connect or exit
				if (_clients_change) {
					// record the maximum number of fd in all scokets
					_maxSock = _clients.begin()->second->getSockfd();
				
					for (auto iter : _clients) {
						FD_SET(iter.second->getSockfd(), &fdRead);
						if (_maxSock < iter.second->getSockfd()) _maxSock = iter.second->getSockfd();
					}
					
					// back up an new file descriptor set
					memcpy(&_fdRead_pre, &fdRead, sizeof(fd_set));
					_clients_change = false;
				}
				else {
					memcpy(&fdRead, &_fdRead_pre, sizeof(fd_set));
				}
				 
				// fisrt arg: ignore, the nfds parameter is included only for compatibility with Berkeley sockets, to present to largest file descriptor number.
				// last arg is timeout: The maximum time for select to wait for checking status of sockets
				// allow a program to monitor multiple file descriptors, waiting until one or more of the file descriptors become "ready" for some class of I/O operation
				// when select find status of sockets change, it would clear all sockets and reload the sockets which has changed the status
				timeval t = { 1, 0 };

				int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, &t);

				if (ret == 0) continue;

				// error happens when return value less than 0
				if (ret < 0) {
					std::cout << "=================" << std::endl;
					std::cout << "Exception happens" << std::endl;
					std::cout << "=================" << std::endl;
					closeSock();
					return;
				}

#				ifdef _WIN32
				// loop through all client sockets to process command
				for (int n = 0; n < fdRead.fd_count; n++) {
					// fd array is a socket array in windows, while in unix it is a bitmask
					auto iter = _clients.find(fdRead.fd_array[n]);

					if (FD_ISSET(iter->second->getSockfd(), &fdRead)) {
						if (iter != _clients.end()) {
							if (RecvData(iter->second) == -1) {
								if (_netEvent) _netEvent->OnExit(iter -> second);
								std::cout << "Client " << iter->second->getSockfd() << " exit" << std::endl;

								_clients_change = true;
								_clients.erase(iter->first);
							}
						}
						else {
							std::cout << "error, if (iter != _clients.end())" << std::endl;
						}
					}
				}

#				else
				std::vector<ClientSocket*> temp;
				for (auto iter : _clients) {
					if (FD_ISSET(iter.second->getSockfd(), &fdRead)) {
						if (RecvData(iter.second) == -1) {
							if (_netEvent) _netEvent->OnExit(iter.second);
							std::cout << "Client " << iter.second->getSockfd() << " exit" << std::endl;

							_clients_change = true;
							temp.push_back(iter.second);
						}
					}
				}

				for (auto client : temp) {
					_clients.erase(client->sockfd());
					delete client;
				}
# 				endif
				//std::cout << "Server is idle and able to deal with other tasks" << std::endl;
			}
		}

		// receive client message, solve message concatenation
		int RecvData(ClientSocket* client) {
			// 5. keeping reading message from clients
			// we only read header info from the incoming message
			
			// pointer points to the client buffer
			char* _szRecv = client->getMsgBuf() + client->getOffset();

			// receive messages from clients and store into buffer
			int nLen = (int)recv(client->getSockfd(), _szRecv, (RECV_BUFF_SIZE * 5) - client->getOffset(), 0);
			
			// increase number of received packages
			_netEvent->OnNetRecv(client);

			// std::cout << "nLen = " << nLen << std::endl;
			if (nLen <= 0) {
				// connection has closed
				//std::cout << "Client " << client->getSockfd() << " closed" << std::endl;
				return -1;
			}

			// copy all messages from the received buffer to second buffer 
			// memcpy(client->getMsgBuf() + client->getOffset(), _szRecv, nLen);

			// increase offset so that the next message will be moved to the end of the previous message
			// TODO: reconsider the value to set
			client->setOffset(client->getOffset() + nLen);

			// receive at least one full dataheader, 
			// repeatedly process the incoming message, which solve packet concatenation

			while (client->getOffset() >= sizeof(DataHeader)) {
				DataHeader* header = (DataHeader*)client->getMsgBuf();

				// receive a full message including data header
				if (client->getOffset() >= header->length) {

					// the length of all following messages
					int shiftLen = client->getOffset() - header->length;

					// response with client message
					OnNetMsg(client, header);

					// successfully processs the first message
					// shift all following messages to the beginning of second buffer
					memcpy(client->getMsgBuf(), client->getMsgBuf() + header->length, shiftLen);

					client->setOffset(shiftLen);
				}
				else {
					// the remaining message is not complete, wait until we get a full next message
					break;
				}
			}

			return 0;
		}

		// response client message, there can be different ways of processing messages in different kinds of server
		// we use virutal to for inheritance
		virtual void OnNetMsg(ClientSocket* client, DataHeader* header) {
			// increase the count of received message
			_netEvent->OnNetMsg(client, header);
		}
	
		// add client from main thread into the buffer queue of child thread
		void addClient(ClientSocket* client) {
			//_mutex.lock();
			std::lock_guard<std::mutex> lock(_mutex);
			_clients_Buffer.push_back(client);
			//_mutex.unlock();
		}

		void start() {
			// TODO: review this function
			// start an thread for child server, to listen and process client message
			_pThread = std::thread(std::mem_fun(&ChildServer::listenClient), this);
		}

		size_t getCount() {
			return _clients.size() + _clients_Buffer.size();
		}

		void setMainServer(INetEvent* event) {
			_netEvent = event;
		}

		~ChildServer() {
			closeSock();
		}

	private:
		// server socket
		SOCKET _sock;

		// all client sockets connected with server, 
		// we allocate its memory on heap to avoid stack overflow
		// since each size of object is large
		std::map<SOCKET, ClientSocket*> _clients;

		// buffer queue to store clients sent from main thread
		std::vector<ClientSocket*> _clients_Buffer;

		// mutex for accessing buffer queue
		std::mutex _mutex;

		// thread of child server
		std::thread _pThread;

		// pointer points to main server, which can be used to call onExit() 
		// to delete the number of connected clients
		INetEvent* _netEvent;
};

class EasyTcpServer : public INetEvent
{
public:
	EasyTcpServer() :_sock{ INVALID_SOCKET }, _clients_list{}, _time{}, _recvCount{ 0 }, _msgCount{ 0 }, _clientCount { 0 }, _child_servers{} {}

	// initialize server socket
	SOCKET initSocket() {
#		ifdef _WIN32
		// launch windows socket 2.x environment
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;

		// initiates use of the Winsock DLL by program.
		WSAStartup(ver, &dat);
#		endif

		// 1.build a socket
		// first argument: address family
		// second: socket type 
		// third: protocol type
		// socket has been created, close it and create an new one
		if (INVALID_SOCKET != _sock) {
			std::cout << "Socket: " << _sock << " has been created previously, close it and recreate an new socket" << std::endl;
			closeSock();
		}

		// 1. build socket
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			std::cout << "Socket create failed" << std::endl;
			return -1;
		}
		else {
			std::cout << "Socket " << _sock << " create succeed" << std::endl;
		}

		// return socket number
		return _sock;
	}

	// bind ip and port
	int bindPort(const char* ip, unsigned short port) {
		if (INVALID_SOCKET == _sock) {
			std::cout << "server socket did not created, create socket automatically instead" << std::endl;
			initSocket();
		}

		// same struct compared with the standard argument type of bind function, we use type conversion to convert its type
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;

		// host to net unsigned short (from int to ushort)
		_sin.sin_port = htons(port);

		// bind ip address ,
		// if use 127.0.0.1, the socket will listen for incoming connections only on the loopback interface using that specific IP address
		// if use ip address of LAN, socket will be bound to that address with specficied port, and can listen to any packets from LAN or public network
		//inet_addr("127.0.0.1");
#		ifdef _WIN32
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#		else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#		endif

		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));

		if (SOCKET_ERROR == ret) {
			std::cout << "ERROR, cannot bind to port: " << port << std::endl;
		}
		else {
			std::cout << "Successfully bind to port: " << port << std::endl;
		}

		return ret;

	}

	// defines the maximum length to the queue of pending connections
	int listenNumber(int n) {
		int ret = listen(_sock, n);

		if (SOCKET_ERROR == ret) {
			std::cout << "ERROR, socket " << _sock << " listen to port failed" << std::endl;
		}
		else {
			std::cout << "Socket: " << _sock << " listen to port successully" << std::endl;
		}

		return ret;
	}

	// accept client connection
	SOCKET acceptClient() {
		// 4. wait until accept an new client connection
		// The accept function fills this structure with the address information of the client that is connecting.
		sockaddr_in clientAddr = {};

		// After the function call, it will be updated with the actual size of the client's address information.
		int nAddrLen = sizeof(clientAddr);

		SOCKET cSock = INVALID_SOCKET;

#		ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#		else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#		endif

		if (INVALID_SOCKET == cSock) {
			std::cout << "ERROR:Invalid Socket " << _sock << " accepted" << std::endl;
		}
		else {
			// send message to all client that there is an new client connected to server
			NewUserJoin client;
			client.cSocket = cSock; 
			//broadcastMessage(&client);

			ClientSocket* newCLient = new ClientSocket(cSock);

			if (newCLient == nullptr) {
				std::cout << "ERROR: CLient socket create failed" << std::endl;

#				ifdef _WIN32
				// close server socket
				closesocket(cSock);
#				else
				close(cSock);
#				endif

				return INVALID_SOCKET;
			}

			// choose a child server which has least clients
			addClientToChild(newCLient);
		}

		return cSock;
	}

	void addClientToChild(ClientSocket* client) {
		OnJoin(client);
		
		// Least connection method to assign client to one of the child server
		auto minClientServer = _child_servers[0];

		for (auto childServer : _child_servers) {
			if (minClientServer->getCount() > childServer->getCount()) {
				minClientServer = childServer;
			}
		} 

		minClientServer->addClient(client); 
	}

	// listen client message
	bool onRun() {
		if (!isRun()) {
			return false;
		}

		recvMsgRate();

		// fd_set: to place sockets into a "set" for various purposes, such as testing a given socket for readability using the readfds parameter of the select function
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;

		// reset the count of each set to zero
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		// add socket _sock to each set to be monitor later when using select()
		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);

		// setup time stamp to listen client connection, that is, 
		// our server is non-blocking, and can process other request while listening to client socket
		// set time stamp to 10 millisecond to check if there are any connections from client
		timeval t = { 0, 10 };

		// fisrt arg: ignore, the nfds parameter is included only for compatibility with Berkeley sockets.
		// last arg is timeout: The maximum time for select to wait for checking status of sockets
		// allow a program to monitor multiple file descriptors, waiting until one or more of the file descriptors become "ready" for some class of I/O operation
		// when select find status of sockets change, it would clear all sockets and reload the sockets which has changed the status

		// drawback of select function: maximum size of fdset is 64, which means, there can be at most 64 clients connected to server, we already reset the size of fdset to 1024

		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &t);

		// error happens when return value less than 0
		if (ret < 0) {
			std::cout << "=================" << std::endl;
			std::cout << "Exception happens" << std::endl;
			std::cout << "=================" << std::endl;
			closeSock();
			return false;
		}

		// the status of server socket is changed, which is, a client is trying to connect with server 
		// create an new socket for current client and add it into client list
		if (FD_ISSET(_sock, &fdRead)) {
			// Clears the bit for the file descriptor fd in the file descriptor set fdRead, so that we can .
			FD_CLR(_sock, &fdRead);

			// accept connection from client
			acceptClient();
		}

		return true;
		//std::cout << "Server is idle and able to deal with other tasks" << std::endl;
	}

	 //start child server to process client message
	void Start(int childCount) {
		// TODO: make sure sock is initialized

		for (int n = 0; n < childCount; n++) {
			auto cServer = new ChildServer(_sock);
			_child_servers.push_back(cServer);
			cServer->setMainServer(this);
			cServer -> start();
		}
	}

	// shutdown child server
	void closeSock() {
		if (_sock == INVALID_SOCKET) {
			return;
		}

#		ifdef _WIN32
		// close all client sockets
		for (int n = 0; n < (int)_clients_list.size(); n++) {
			closesocket(_clients_list[n]->getSockfd());
			// TODO: need to check if it is not nullptr and delete successfully
			delete _clients_list[n];
		}
		// close server socket
		closesocket(_sock);
		// terminates use of the Winsock 2 DLL (Ws2_32.dll)
		WSACleanup();
#		else
		for (int n = 0; n < (int)_clients_list.size(); n++) {
			close(_clients_list[n]->getSockfd());
			delete _clients_list[n];
		}
		close(_sock);
#		endif

		_clients_list.clear();
	}

	// check if socket is created
	bool isRun() {
		return _sock != INVALID_SOCKET;
	}

	// calculate number of packages/messages received per second
	void recvMsgRate() {
		auto t = _time.getElapsedSecond();

		if (t >= 1.0) {
			std::cout << "Threads: " << _child_servers.size() << " - ";
			std::cout << "Clients: " << _clientCount << " - ";
			std::cout << std::fixed << std::setprecision(6) << t << " second, server socket <" << _sock;
			std::cout << "> receive " << (int)(_recvCount / t) << " packets, " << int(_msgCount / t)<< " messages" << std::endl;
			
			_msgCount = 0;
			_recvCount = 0;
			_time.update();
		}
	}

	// send message to client
	int sendMessage(SOCKET cSock, DataHeader* header) {
		if (isRun() && header) {
			return send(cSock, (const char*)header, header->length, 0);
		}

		return SOCKET_ERROR;
	}

	// broadcast message to all users in server
	void broadcastMessage(DataHeader* header) {
		if (isRun() && header) {
			for (int n = (int)_clients_list.size() - 1; n >= 0; n--) {
				sendMessage(_clients_list[n]->getSockfd(), header);
			}
		}
	}

	// increase number of received packages
	virtual void OnNetMsg(ClientSocket* clientSock, DataHeader* header) {
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

		if(iter != _clients_list.end()) _clients_list.erase(iter);

		_clientCount--;
	}

	virtual ~EasyTcpServer() {
		closeSock();
	}

protected:
	// number of received packages
	std::atomic<int> _recvCount;
	
	// number of connected clients 
	std::atomic<int> _clientCount;

	// number of received messages
	std::atomic<int> _msgCount;

	// all client sockets connected with server, this clients list can be used for message broadcast
	std::vector<ClientSocket*> _clients_list;

private:
	// server socket
	SOCKET _sock;

	// child threads to process client messages
	std::vector<ChildServer*> _child_servers;

	CELLTimestamp _time;
};

bool isRun = true;

void cmdThread() {
	while (true) {
		char cmdBuf[256] = {};
		std::cin >> cmdBuf;
		if (strcmp(cmdBuf, "exit") == 0) {
			std::cout << "sub thread finished" << std::endl;
			// tell main thread that the sub thread is finished
			isRun = false;
			break;
		}
		else {
			std::cout << "not valid command" << std::endl;
		}
	}
}


#endif // !_EsayTcpServer_hpp
