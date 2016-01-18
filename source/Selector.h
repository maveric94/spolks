#ifndef SELECTOR_H
#define SELECTOR_H

#include "TcpListener.h"
#include "TCPSocket.h"
#include "UDPSocket.h"
#include "TypeDefs.h"
#include "util.h"

class Selector
{
public:
	static bool isTCPSocketReadyToReceive(TCPSocket* socket, Int32 millisecs)
	{
		return isSocketReadyToReceive(socket->mSocket, millisecs);
	}
	static bool isTCPSocketReadyToSend(TCPSocket* socket, Int32 millisecs)
	{
		return isSocketReadyToSend(socket->mSocket, millisecs);
	}
	static bool isUDPSocketReadyToReceive(UDPSocket* socket, Int32 millisecs)
	{
		return isSocketReadyToReceive(socket->mSocket, millisecs);
	}
	static bool isUDPSocketReadyToSend(UDPSocket* socket, Int32 millisecs)
	{
		return isSocketReadyToSend(socket->mSocket, millisecs);
	}
	static bool isListenerReady(TCPListener* listener, Int32 millisecs)
	{
		return isSocketReadyToReceive(listener->mSocket, millisecs);
	}

private:
	static bool isSocketReadyToSend(Socket socket, Int32 millisecs)
	{
		fd_set fd;
		timeval tv;

		FD_ZERO(&fd);
		FD_SET(socket, &fd);

		tv.tv_sec = millisecs / 1000;
		tv.tv_usec = (millisecs % 1000) * 1000;

		Int32 selectResult = select(socket + 1, NULL, &fd, NULL, &tv);
		if (selectResult <= 0)
			return false;
		else
			return true;
	}
	static bool isSocketReadyToReceive(Socket socket, Int32 millisecs)
	{
		fd_set fd;
		timeval tv;

		FD_ZERO(&fd);
		FD_SET(socket, &fd);

		tv.tv_sec = millisecs / 1000;
		tv.tv_usec = (millisecs % 1000) * 1000;

		Int32 selectResult = select(socket + 1, &fd, NULL, NULL, &tv);
		if (selectResult <= 0)
			return false;
		else
			return true;
	}
};

#endif