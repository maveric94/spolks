#ifndef TCPLISTENER_H
#define TCPLISTENER_H

#include "Socket.h"
#include "TCPSocket.h"

class TCPListener
{
public:
	TCPListener();

	int Bind(const char* port);
	int Listen();
	TCPSocket* Accept();
	int Close();

private:
	TCPListener& operator = (TCPListener socket);
	TCPListener(TCPListener& socket);

	Socket mSocket;
	addrinfo* mAddrInfo;
};

#endif