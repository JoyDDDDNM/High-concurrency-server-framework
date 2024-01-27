#include "Client.hpp"

Client::Client(SOCKET sockfd = INVALID_SOCKET) :_sockfd{ sockfd }, _szMsgBuf{ {} }, _offset{ 0 }, _lastSendPos{ 0 } {
	memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
	memset(_szSendBuf, 0, SEND_BUFF_SIZE);
}

SOCKET Client::getSockfd() {
	return _sockfd;
}

char* Client::getMsgBuf() {
	return _szMsgBuf;
}

int Client::getOffset() {
	return _offset;
}

void Client::setOffset(int pos) {
	_offset = pos;
}

// send messages to clients
int Client::sendMessage(DataHeaderPtr& header) {
	int ret = SOCKET_ERROR;

	int nSendLen = header->length;
	const char* pSendData = (const char*)header.get();

	while (true) {
		// reach buffer size limit
		if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE) {
			// count number of messages we can send
			int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;

			// only copy certain amount of next message to fill the buffer
			memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);

			// increase pointer 
			pSendData += nCopyLen;

			// decrease len of next message to be copied into buffer
			nSendLen -= nCopyLen;

			// send messages when receive large enough messages
			ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);

			// reset offset
			_lastSendPos = 0;

			if (ret == SOCKET_ERROR) return ret;
		}
		else {
			memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
			_lastSendPos += nSendLen;
			break;
		}
	}

	return ret;
}
