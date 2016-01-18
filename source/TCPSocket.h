#ifndef TCPSocket_H
#define TCPSocket_H

#include "util.h"
#include "Packet.h"
#include "TypeDefs.h"

#include <string>

class TCPSocket
{
	friend class Selector;
public:

	//typedef std::unique_ptr<TCPSocket> TCPSocketPtr;


	TCPSocket();
	TCPSocket(Socket rawSocket);
	~TCPSocket();
	Int32 Connect(const char* address, const char* port);
	Int32 Send(char* data, size_t size, Int32 timeout = -1, bool isOOB = false);
	Int32 Send(Packet& packet, Int32 timeout = -1);
	Int32 Receive(char* buffer, size_t size, Int32 timeout = -1, bool isOOB = false);
	Int32 Receive(Packet& packet, Int32 timeout = -1);
	Int32 SendOOBByte(char byte);
	Int32 ReceiveOOBByte(char& byte);
	Int32 Close();
	Int32 Reconnect();
	bool IsConnected();

	Int32 GetSocketRemoteAddress(std::string& address, Int32& port);

	bool isTimeoutEnable;


	UInt64 mBytesReceived, mBytesSend;

private:
	/*TCPSocket& operator = (TCPSocket socket);
	TCPSocket(TCPSocket& socket);*/

	addrinfo* mAddrInfo;
	bool mIsConnected;
	Socket mSocket;
};

#endif