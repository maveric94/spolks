#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include "Socket.h"
#include "Packet.h"

class TCPSocket
{
public:

	typedef std::unique_ptr<TCPSocket> TCPSocketPtr;


	TCPSocket();
	TCPSocket(Socket rawSocket);
	~TCPSocket();
	int Connect(const char* address, const char* port);
	int Send(char* data, size_t size, int timeout = -1, bool isOOB = false);
	int Send(Packet& packet, int timeout = -1);
	int Receive(char* buffer, size_t size, int timeout = -1, bool isOOB = false);
	int Receive(Packet& packet, int timeout = -1);
	int SendOOBByte(char byte);
	int ReceiveOOBByte(char& byte);
	int Close();
	int Reconnect();
	bool IsConnected();

private:
	/*TCPSocket& operator = (TCPSocket socket);
	TCPSocket(TCPSocket& socket);*/

	Socket mSocket;
	addrinfo* mAddrInfo;
	bool mIsConnected;
};

#endif