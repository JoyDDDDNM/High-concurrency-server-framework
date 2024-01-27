#ifndef _INET_EVENT_HPP_
#define	_INET_EVENT_HPP_

#include "Cell.hpp"
#include "Client.hpp"

class ChildServer;

// network event interface
class INetEvent {
public:
	INetEvent() = default;

	// client exits server
	virtual void OnJoin(ClientPtr& clientSock) = 0;
	virtual void OnExit(ClientPtr& clientSock) = 0;
	virtual void OnNetMsg(ChildServer* pChildServer, ClientPtr& clientSock, DataHeaderPtr header) = 0;
	virtual void OnNetRecv(ClientPtr& clientSock) = 0;
	~INetEvent() = default;

private:

};

#endif // !_INET_EVENT_HPP_
