#ifndef _CLIENT_HPP_
#define _CLIENT_HPP_

#include "Cell.hpp"
#include "ObjectPool.hpp"
#include "Message.hpp"

#include <memory>

// client socket info, we can accept up to 10_000 clients at the same time
class Client : public ObjectPoolBase<Client, 10000> {
public:
	Client(SOCKET sockfd);

	SOCKET getSockfd();

	char* getMsgBuf();

	int getOffset();

	void setOffset(int pos);

	// send messages to clients
	int sendMessage(DataHeaderPtr& header);

private:
	// socket fd, which will be put into selcet function
	SOCKET _sockfd;

	// buffer which stores data after we receive it from the buffer inside OS kernel
	char _szMsgBuf[RECV_BUFF_SIZE];

	// offset pointer pointing to the end of messages received from _szRecv
	int _offset;

	// buffer which stores messages which would be sent to clients later
	char _szSendBuf[SEND_BUFF_SIZE];

	// offset pointers pointing to the end end of messages received from _szSendBuf
	int _lastSendPos;
};

using ClientPtr = std::shared_ptr<Client>;

#endif