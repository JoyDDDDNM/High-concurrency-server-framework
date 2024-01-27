#include "ChildServer.hpp"

#include <functional>

ChildServer::ChildServer(SOCKET sock = INVALID_SOCKET) :_sock{ sock }, _clients{}, _clients_Buffer{}, _mutex{}, _thread{}, _pNetEvent{ nullptr } {}

// check if socket is creaBted
bool ChildServer::isRun() {
	return _sock != INVALID_SOCKET;
}

// close socket
void ChildServer::closeSock() {
	if (_sock == INVALID_SOCKET) return;

#		ifdef _WIN32
	// close all client sockets
	for (auto iter : _clients) {
		closesocket(iter.second->getSockfd());
		// TODO: need to check if it is not nullptr and delete successfully
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

// keep running to listen client message
void ChildServer::OnRun() {
	_clients_change = true;
	while (isRun()) {
		// check if buffer queue contain any connected clients
		if (!_clients_Buffer.empty()) {
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
						if (_pNetEvent) _pNetEvent->OnExit(iter->second);
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
		std::vector<ClientPtr> temp;
		for (auto iter : _clients) {
			if (FD_ISSET(iter.second->getSockfd(), &fdRead)) {
				if (RecvData(iter.second) == -1) {
					if (_pNetEvent) _pNetEvent->OnExit(iter.second);
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
int ChildServer::RecvData(ClientPtr& client) {
	// 5. keeping reading message from clients
	// we only read header info from the incoming message

	// pointer points to the client buffer
	char* _szRecv = client->getMsgBuf() + client->getOffset();

	// receive messages from clients and store into buffer
	int nLen = (int)recv(client->getSockfd(), _szRecv, (RECV_BUFF_SIZE)-client->getOffset(), 0);

	// increase number of received packages
	_pNetEvent->OnNetRecv(client);

	if (nLen <= 0) {
		// connection has closed
		//std::cout << "Client " << client->getSockfd() << " closed" << std::endl;
		return -1;
	}

	// increase offset so that the next message will be moved to the end of the previous message
	// TODO: reconsider the value to set
	client->setOffset(client->getOffset() + nLen);

	// receive at least one full dataheader, 
	// repeatedly process the incoming message, which solve packet concatenation

	while (client->getOffset() >= sizeof(DataHeader)) {
		DataHeader* ptr = (DataHeader*)client->getMsgBuf();
		DataHeaderPtr header = std::make_shared<DataHeader>(*ptr);

		// receive a full message including data header
		if (client->getOffset() >= header->length) {

			// the length of all following messages
			int shiftLen = client->getOffset() - header->length;

			// get a complete message and response with client 
			OnNetMsg(client, header);

			// successfully processs the first message
			// shift all following messages to the beginning of buffer
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
void ChildServer::OnNetMsg(ClientPtr client, DataHeaderPtr header) {
	// increase the count of received message
	_pNetEvent->OnNetMsg(this, client, header);
}

// add client from main thread into the buffer queue of child thread
void ChildServer::addClient(ClientPtr client) {
	std::lock_guard<std::mutex> lock(_mutex);
	_clients_Buffer.push_back(client);
}

void ChildServer::start() {
	// TODO: review this function
	// start an thread for child server, to listen and process client message
	_thread = std::thread(std::bind(&ChildServer::OnRun,this));
	_thread.detach();

	// start task server to reponse messages
	_taskServer.start();
}

size_t ChildServer::getCount() {
	return _clients.size() + _clients_Buffer.size();
}

void ChildServer::setMainServer(INetEvent* event) {
	_pNetEvent = event;
}

void ChildServer::addSendTask(ClientPtr clientSock, DataHeaderPtr header) {
	CellTaskPtr task = std::make_shared<CellSendMsgToClientTask>(clientSock, header);

	_taskServer.addTask(task);
}

ChildServer::~ChildServer() {
	closeSock();
	_sock = INVALID_SOCKET;
}
