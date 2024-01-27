#include "TcpServer.hpp"

EasyTcpServer::EasyTcpServer() :_sock{ INVALID_SOCKET }, 
								_clients_list{},
								_time{},
								_recvCount{ 0 },
								_msgCount{ 0 },
								_clientCount{ 0 },
								_child_servers{},
								isRunning{ true }
								{}

// initialize server socket
SOCKET EasyTcpServer::initSocket() {
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

	// 1. create socket
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
int EasyTcpServer::bindPort(const char* ip, unsigned short port) {
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
int EasyTcpServer::listenNumber(int n) {
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
SOCKET EasyTcpServer::acceptClient() {
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
		// NewUserJoin client;
		// client.cSocket = cSock; 
		//broadcastMessage(&client);

		// choose a child server which has least clients
		// call the overload new to request memory
		ClientPtr c(new Client(cSock));
		// addClientToChild(std::make_shared<Client>(cSock));
		addClientToChild(c);
	}

	return cSock;
}

void EasyTcpServer::addClientToChild(ClientPtr client) {
	// Least connection method to assign client to one of the child server
	auto minClientServer = _child_servers[0];

	for (auto childServer : _child_servers) {
		if (minClientServer->getCount() > childServer->getCount()) {
			minClientServer = childServer;
		}
	}

	minClientServer->addClient(client);
	OnJoin(client);
}

// listen client message
bool EasyTcpServer::onRun() {
	if (!isRun()) return false;

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

// start child server to process client message
void EasyTcpServer::Start(int childCount) {
	if (_sock == INVALID_SOCKET) {
		std::cout << "please initialize server socket before lanuching" << std::endl;
		return;
	}

	for (int n = 0; n < childCount; n++) {
		auto cServer = std::make_shared<ChildServer>(_sock);
		_child_servers.push_back(cServer);
		cServer->setMainServer(this);
		cServer->start();
	}
}

// shutdown child server
void EasyTcpServer::closeSock() {
	if (_sock == INVALID_SOCKET) {
		std::cout << "Server is not running" << std::endl;
		return;
	}

#		ifdef _WIN32
	// close server socket
	closesocket(_sock);
	// terminates use of the Winsock 2 DLL (Ws2_32.dll)
	WSACleanup();
#		else
	close(_sock);
#		endif

	_clients_list.clear();
}

// check if socket is created
bool EasyTcpServer::isRun() {
	return (_sock != INVALID_SOCKET) && isRunning;
}

// calculate number of packages/messages received per second
void EasyTcpServer::recvMsgRate() {
	auto t = _time.getElapsedSecond();

	if (t >= 1.0) {
		std::cout << "Threads: " << _child_servers.size() << " - ";
		std::cout << "Clients: " << _clientCount << " - ";
		std::cout << std::fixed << std::setprecision(6) << t << " second, server socket <" << _sock;
		std::cout << "> receive " << (int)(_recvCount / t) << " packets, " << int(_msgCount / t) << " messages" << std::endl;

		_msgCount = 0;
		_recvCount = 0;
		_time.update();
	}
}

// send message to client
int EasyTcpServer::sendMessage(SOCKET cSock, DataHeader* header) {
	if (isRun() && header) {
		return send(cSock, (const char*)header, header->length, 0);
	}

	return SOCKET_ERROR;
}

// broadcast message to all users in server
void EasyTcpServer::broadcastMessage(DataHeader* header) {
	if (isRun() && header) {
		for (int n = (int)_clients_list.size() - 1; n >= 0; n--) {
			sendMessage(_clients_list[n]->getSockfd(), header);
		}
	}
}

// increase number of received packages
void EasyTcpServer::OnNetMsg(ChildServer* pChildServer, ClientPtr& clientSock, DataHeaderPtr header) {
	_msgCount++;
}

void EasyTcpServer::OnNetRecv(ClientPtr& clientSock) {
	_recvCount++;
}

// new client connect server
void EasyTcpServer::OnJoin(ClientPtr& clientSock) {
	_clients_list.push_back(clientSock);
	_clientCount++;
}

// delete the socket of exited client
void EasyTcpServer::OnExit(ClientPtr& clientSock) {
	auto iter = _clients_list.end();

	for (int n = (int)_clients_list.size() - 1; n >= 0; n--) {
		if (_clients_list[n] == clientSock) {
			iter = _clients_list.begin() + n;
		}
	}

	if (iter != _clients_list.end()) _clients_list.erase(iter);

	_clientCount--;
}

EasyTcpServer::~EasyTcpServer() {
	closeSock();
}

void cmdThread(EasyTcpServer& Server) {
	while (true) {
		char cmdBuf[256] = {};
		std::cin >> cmdBuf;
		if (strcmp(cmdBuf, "exit") == 0) {
			std::cout << "sub thread finished" << std::endl;
			// tell main thread that the sub thread is finished
			Server.isRunning = false;
			break;
		}
		else {
			std::cout << "not valid command" << std::endl;
		}
	}
}