#include "TCPSocket.h"
#include <iostream>
TCPSocket::TCPSocket()
{
	mSocket = INVALID_SOCKET;
	mIsConnected = false;
	mAddrInfo = NULL;

	mBytesReceived = 0;
	mBytesSend = 0;

	isTimeoutEnable = false;
}

TCPSocket::~TCPSocket()
{
	/*if (!mIsConnected)
		Close();*/
	if (mAddrInfo != NULL)
		freeaddrinfo(mAddrInfo);
}

TCPSocket::TCPSocket(Socket rawSocket)
{
	mSocket = rawSocket;
	mIsConnected = true;
	mAddrInfo = NULL;

	mBytesReceived = 0;
	mBytesSend = 0;

	isTimeoutEnable = false;

}

Int32 TCPSocket::Connect(const char* address, const char* port)
{
	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = GET_TCP_PROTO;

	Int32 result = getaddrinfo(address, port, &hints, &mAddrInfo);

	return Reconnect();
}

Int32 TCPSocket::Close()
{
	mIsConnected = false;
	//freeaddrinfo(mAddrInfo);
	Int32 retval = (mSocket == INVALID_SOCKET) ? 0 : CLOSE_SOCKET(mSocket);
	mSocket = INVALID_SOCKET;
	return retval;
}

Int32 TCPSocket::Send(char* data, size_t size, Int32 timeout, bool isOOB)
{
	Int32 flags = isOOB ? MSG_OOB : 0;
	if (timeout != -1)
	{
		fd_set fd;
		timeval tv;

		FD_ZERO(&fd);
		FD_SET(mSocket, &fd);

		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		Int32 selectResult = select(mSocket + 1, NULL, &fd, NULL, &tv);
		if (selectResult <= 0)
			return selectResult;

	}

	return send(mSocket, data, (Int32)size, flags);

}
Int32 TCPSocket::Receive(char* buffer, size_t size, Int32 timeout, bool isOOB)
{
	Int32 flags = isOOB ? MSG_OOB : 0;
	Int32 bytesLeft = size;

	do
	{
		if (timeout != -1)
		{
			fd_set fd;
			timeval tv;

			FD_ZERO(&fd);
			FD_SET(mSocket, &fd);

			tv.tv_sec = timeout;
			tv.tv_usec = 0;

			Int32 selectResult = select(mSocket + 1, &fd, NULL, NULL, &tv);
			if (selectResult <= 0)
				return selectResult;
		}
		Int32 bytesReceived;

		bytesReceived = recv(mSocket, buffer + size - bytesLeft, bytesLeft, flags);

		if (bytesReceived <= 0)
			return bytesReceived;

		bytesLeft -= bytesReceived;
	} while (bytesLeft);

	return (Int32)size;
}

Int32 TCPSocket::Send(Packet& packet, Int32 timeout)
{
	Int32 packetSize = packet.mData.size();
	Int32 opResult;
	Packet initPacket;
	initPacket << packetSize;

	opResult = Send(&initPacket.mData[0], initPacket.mData.size() * sizeof(char), timeout);
	if (opResult <= 0)
		return opResult;
	opResult = Send(&packet.mData[0], packet.mData.size() * sizeof(char), timeout);
	if (opResult <= 0)
		return opResult;

	//std::cout << opResult << " bytes sent.\n";
	mBytesSend += opResult;

	return opResult;
}
Int32 TCPSocket::Receive(Packet& packet, Int32 timeout)
{
	Int32 packetSize;
	Int32 opResult;
	Packet initPacket;

	initPacket.mData.resize(sizeof(Int32));
	opResult = Receive(&initPacket.mData[0], sizeof(Int32), timeout);
	if (opResult <= 0)
		return opResult;

	initPacket >> packetSize;

	packet.Clear();
	packet.mData.resize(packetSize);
	opResult = Receive(&packet.mData[0], packetSize, timeout);
	if (opResult <= 0)
		return opResult;

	//std::cout << opResult << " bytes received.\n";
	mBytesReceived += opResult;

	return opResult;
}

Int32 TCPSocket::SendOOBByte(char byte)
{
	/*Int32 result;
	Timeval tv;

	setTimeval(tv, 10);
	setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)(void*)&tv, sizeof(Timeval));

	result = Send(&byte, 1, -1, true);

	setTimeval(tv, 300000); //5 mins
	setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)(void*)&tv, sizeof(Timeval));
	return result;*/

	return Send(&byte, 1, -1, true);
}
Int32 TCPSocket::ReceiveOOBByte(char& byte)
{
	/*Int32 result;
	Timeval tv;

	setTimeval(tv, 10);
	setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)(void*)&tv, sizeof(Timeval));

	result = Receive(&byte, 1, -1, true);

	setTimeval(tv, 300000); //5 mins
	Int32 res = setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)(void*)&tv, sizeof(Timeval));
	return result;*/

	Int32 result;
	timeval tv;
	fd_set fd;

	FD_ZERO(&fd);
	FD_SET(mSocket, &fd);

	tv.tv_usec = 10;
	tv.tv_sec = 0;

	result = select(mSocket + 1, NULL, NULL, &fd, &tv);
	if (result <= 0)
		return result;

	result = Receive(&byte, 1, -1, true);

	return result;
}

Int32 TCPSocket::Reconnect()
{
	for (addrinfo* ptr = mAddrInfo; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		mSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (mSocket == INVALID_SOCKET)
		{
			mIsConnected = false;
			return -1;
		}

		// Connect to server.
		if (connect(mSocket, ptr->ai_addr, (Int32)ptr->ai_addrlen) == SOCKET_ERROR)
		{
			Close();
			continue;
		}
		break;
	}
	if (mSocket == INVALID_SOCKET)
	{
		mIsConnected = false;
		return -1;
	}
		

	mIsConnected = true;
	return 0;
}

bool TCPSocket::IsConnected()
{
	return mIsConnected;
}

Int32 TCPSocket::GetSocketRemoteAddress(std::string& address, Int32& port)
{
	sockaddr addr;
	socklen_t size = sizeof(sockaddr);
	char ip[INET6_ADDRSTRLEN];

	if (!getpeername(mSocket, &addr, &size))
	{
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;
		port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ip, sizeof ip);
		address = ip;
		return 0;
	}

	return -1;
}




