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

int TCPSocket::Send(char* data, size_t size)
{
	return send(mSocket, data, (int)size, 0);
}
int TCPSocket::Receive(char* buffer, size_t size)
{
	return recv(mSocket, buffer, (int)size, MSG_WAITALL);
}

int TCPSocket::Send(Packet& packet)
{
	Int32 packetSize = packet.mData.size();
	Int32 opResult;
	Packet initPacket;
	initPacket << packetSize;

	opResult = send(mSocket, &initPacket.mData[0], initPacket.mData.size() * sizeof(char), 0);
	if (opResult <= 0)
		return opResult;
	opResult = send(mSocket, &packet.mData[0], packet.mData.size() * sizeof(char), 0);
	if (opResult <= 0)
		return opResult;

	if (opResult > 0)
		std::cout << opResult << " bytes sent.\n";
	else
		std::cout << "Error occured while sendind data.\n";

	return opResult;
}
int TCPSocket::Receive(Packet& packet)
{
	Int32 packetSize;
	Int32 opResult;
	Packet initPacket;

	initPacket.mData.resize(sizeof(Int32));
	opResult = recv(mSocket, &initPacket.mData[0], sizeof(Int32), MSG_WAITALL);
	if (opResult <= 0)
		return opResult;
	initPacket >> packetSize;

	packet.Clear();
	packet.mData.resize(packetSize);
	opResult = recv(mSocket, &packet.mData[0], packetSize, MSG_WAITALL);
	if (opResult > 0)
		std::cout << opResult << " bytes received.\n";
	else
		std::cout << "Error occured while receiveing data.\n";

	return opResult;
}

int TCPSocket::Reconnect()
{
	for (addrinfo* ptr = mAddrInfo; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		mSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (mSocket == INVALID_SOCKET)
			return -1;

		// Connect to server.
		if (connect(mSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
		{
			Close();
			continue;
		}
		break;
	}
	if (mSocket == INVALID_SOCKET)
		return -1;

	mIsConnected = true;
	return 0;
}

bool TCPSocket::IsConnected()
{
	return mIsConnected;
}

