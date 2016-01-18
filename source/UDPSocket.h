#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include "util.h"
#include "Packet.h"
#include "TypeDefs.h"

class UDPSocket
{
	friend class Selector;
public:

	UDPSocket();
	~UDPSocket();

	Int32 Bind(const char* port);

	Int32 SendTo(Packet& packet, sockaddr_in* addr);
	Int32 ReceiveFrom(Packet& packet, sockaddr_in* addr);

	Int32 ReceiveFrom(char* buffer, size_t size, sockaddr_in* addr);
	Int32 SendTo(char* data, size_t size, sockaddr_in* addr);

	Int32 Close();

	bool isTimeoutEnable;

private:

	addrinfo* mAddrInfo;
	bool mIsConnected;
	Socket mSocket;
};

#endif