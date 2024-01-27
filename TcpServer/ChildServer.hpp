#ifndef _CHILD_SERVER_HPP_
#define _CHILD_SERVER_HPP_

#include "Cell.hpp"
#include "Client.hpp"
#include "CELLTask.hpp"
#include "INetEvent.hpp"

#include <map>
#include <vector>
#include <thread>

class ChildServer {
public:
	using CellTaskPtr = std::shared_ptr<CellTask>;
	using CellSendMsgToClientTaskptr = std::shared_ptr<CellSendMsgToClientTask>;

	ChildServer(SOCKET sock);

	// check if socket is creaBted
	bool isRun();

	// close socket
	void closeSock();

	// keep running to listen client message
	void OnRun();

	// receive client message, solve message concatenation
	int RecvData(ClientPtr& client);

	// response client message, there can be different ways of processing messages in different kinds of server
	// we use virutal to for inheritance
	virtual void OnNetMsg(ClientPtr client, DataHeaderPtr header);

	// add client from main thread into the buffer queue of child thread
	void addClient(ClientPtr client);

	void start();

	size_t getCount();

	void setMainServer(INetEvent* event);

	void addSendTask(ClientPtr clientSock, DataHeaderPtr header);

	~ChildServer();

private:
	// server socket
	SOCKET _sock;

	// all client sockets connected with server, 
	// we allocate its memory on heap to avoid stack overflow
	// since each size of object is large
	std::map<SOCKET, ClientPtr> _clients;

	// buffer queue to store clients sent from main thread
	std::vector<ClientPtr> _clients_Buffer;

	// mutex for accessing buffer queue
	std::mutex _mutex;

	// thread of child server
	std::thread _thread;

	// backup previous file descriptor set to improve performance
	fd_set _fdRead_pre;

	// check if any clients connect or exit
	bool _clients_change;

	// used in linux environment
	SOCKET _maxSock;

	// pointer points to main server, which can be used to call onExit() 
	// to delete the number of connected clients
	INetEvent* _pNetEvent;

	// subServer for responding messages
	CellTaskServer _taskServer;
};

using ChildServerPtr = std::shared_ptr<ChildServer>;

#endif