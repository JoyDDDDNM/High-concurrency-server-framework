#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>
#include <map>
#include <memory>

#include "Cell.hpp"
#include "ObjectPool.hpp"
#include "Message.hpp"
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"
#include "Client.hpp"
#include "ChildServer.hpp"
#include "INetEvent.hpp"

class EasyTcpServer : public INetEvent{
public:

	EasyTcpServer();

	// initialize server socket
	SOCKET initSocket();

	// bind ip and port
	int bindPort(const char* ip, unsigned short port);

	// defines the maximum length to the queue of pending connections
	int listenNumber(int n);

	// accept client connection
	SOCKET acceptClient();

	void addClientToChild(ClientPtr client);

	// listen client message
	bool onRun();

	 // start child server to process client message
	void Start(int childCount);

	// shutdown child server
	void closeSock();

	// check if socket is created
	bool isRun();

	// calculate number of packages/messages received per second
	void recvMsgRate();

	// send message to client
	int sendMessage(SOCKET cSock, DataHeader* header);

	// broadcast message to all users in server
	void broadcastMessage(DataHeader* header);

	// increase number of received packages
	virtual void OnNetMsg(ChildServer* pChildServer, ClientPtr& clientSock, DataHeaderPtr header) override;

	virtual void OnNetRecv(ClientPtr& clientSock) override;

	// new client connect server
	virtual void OnJoin(ClientPtr& clientSock) override;

	// delete the socket of exited client
	virtual void OnExit(ClientPtr& clientSock) override;

	friend void cmdThread(EasyTcpServer& Server);

	virtual ~EasyTcpServer();

protected:
	// number of received packages
	std::atomic<int> _recvCount;
	
	// number of connected clients 
	std::atomic<int> _clientCount;

	// number of received messages
	std::atomic<int> _msgCount;

	// all client sockets connected with server, this clients list can be used for message broadcast
	std::vector<ClientPtr> _clients_list;


private:
	bool isRunning;

	// server socket
	SOCKET _sock;

	// child server to process client messages
	std::vector<ChildServerPtr> _child_servers;

	CELLTimestamp _time;
};

void cmdThread(EasyTcpServer& Server);

#endif // !_EsayTcpServer_hpp
