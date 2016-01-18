#ifndef TCPLISTENER_H
#define TCPLISTENER_H

#include "util.h"
#include "TCPSocket.h"

class TCPListener
{
	friend class Selector;
public:
	TCPListener();

	Int32 Bind(const char* port);
	Int32 Listen();
	TCPSocket* Accept();
	Int32 Close();

private:
	TCPListener& operator = (TCPListener socket);
	TCPListener(TCPListener& socket);

	Socket mSocket;
	addrinfo* mAddrInfo;
};

#endif