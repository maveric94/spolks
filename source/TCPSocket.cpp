#include "TCPSocket.h"
#include <iostream>
TCPSocket::TCPSocket()
{
	mSocket = INVALID_SOCKET;
	mIsConnected = false;
	mAddrInfo = nullptr;
}

TCPSocket::~TCPSocket()
{
	/*if (!mIsConnected)
		Close();*/
	if (mAddrInfo != nullptr)
		freeaddrinfo(mAddrInfo);
}

TCPSocket::TCPSocket(Socket rawSocket)
{
	mSocket = rawSocket;
	mIsConnected = true;
	mAddrInfo = nullptr;
}

int TCPSocket::Connect(const char* address, const char* port)
{
	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = GET_TCP_PROTO;

	getaddrinfo(address, port, &hints, &mAddrInfo);

	return Reconnect();
}

int TCPSocket::Close()
{
	mIsConnected = false;
	//freeaddrinfo(mAddrInfo);
	int retval = (mSocket == INVALID_SOCKET) ? 0 : CLOSE_SOCKET(mSocket);
	mSocket = INVALID_SOCKET;
	return retval;
}

int TCPSocket::Send(char* data, size_t size, int timeout, bool isOOB)
{
	int flags = isOOB ? MSG_OOB : 0;
	if (timeout != -1)
	{
		fd_set fd;
		timeval tv;

		FD_ZERO(&fd);
		FD_SET(mSocket, &fd);

		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		int selectResult = select(mSocket + 1, nullptr, &fd, nullptr, &tv);
		if (selectResult <= 0)
		{
			Close();
			return selectResult;
		}
	}
	return send(mSocket, data, (int)size, flags);
}
int TCPSocket::Receive(char* buffer, size_t size, int timeout, bool isOOB)
{
	int flags = isOOB ? MSG_OOB : 0;
	int bytesLeft = size;

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

			int selectResult = select(mSocket + 1, &fd, nullptr, nullptr, &tv);
			if (selectResult <= 0)
				return selectResult;
		}
		int bytesReceived = recv(mSocket, buffer + size - bytesLeft, bytesLeft, flags);

		if (bytesReceived <= 0)
		{
			Close();
			return bytesReceived;
		}

		bytesLeft -= bytesReceived;
	} while (bytesLeft);

	return (int)size;
}

int TCPSocket::Send(Packet& packet, int timeout)
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

	if (opResult > 0)
		std::cout << opResult << " bytes sent.\n";
	else
		std::cout << "Error occured while sendind data.\n";

	return opResult;
}
int TCPSocket::Receive(Packet& packet, int timeout)
{
	Int32 packetSize;
	Int32 opResult;
	Packet initPacket;

	initPacket.mData.resize(sizeof(Int32));
	opResult = Receive(&initPacket.mData[0], sizeof(Int32), timeout);
	if (opResult <= 0)
	{
		std::cout << "Error occured while receiveing data.\n";
		return opResult;
	}
	initPacket >> packetSize;

	packet.Clear();
	packet.mData.resize(packetSize);
	opResult = Receive(&packet.mData[0], packetSize, timeout);
	if (opResult > 0)
		std::cout << opResult << " bytes received.\n";
	else
		std::cout << "Error occured while receiveing data.\n";

	return opResult;
}

int TCPSocket::SendOOBByte(char byte)
{
	return Send(&byte, 1, -1, true);
}
int TCPSocket::ReceiveOOBByte(char& byte)
{
	return Receive(&byte, 1, 1, true);
}

int TCPSocket::Reconnect()
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
		if (connect(mSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
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

