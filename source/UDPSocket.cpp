#include "UDPSocket.h"

UDPSocket::UDPSocket()
{
	mSocket = INVALID_SOCKET;
	mAddrInfo = NULL;


	isTimeoutEnable = false;
}

UDPSocket::~UDPSocket()
{
	/*if (!mIsConnected)
	Close();*/
	if (mAddrInfo != NULL)
		freeaddrinfo(mAddrInfo);
}

Int32 UDPSocket::Bind(const char* port)
{
	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = GET_UDP_PROTO;

	getaddrinfo(NULL, port, &hints, &mAddrInfo);
	mSocket = socket(mAddrInfo->ai_family, mAddrInfo->ai_socktype, mAddrInfo->ai_protocol);
	if (mSocket == INVALID_SOCKET)
		return -1;

	return bind(mSocket, mAddrInfo->ai_addr, (Int32)mAddrInfo->ai_addrlen);
}

Int32 UDPSocket::SendTo(char* data, size_t size, sockaddr_in* addr)
{
	Int32 flags = 0;
	/*if (timeout != -1)
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

	}*/

	return sendto(mSocket, data, (Int32)size, flags, (sockaddr*)addr, sizeof(sockaddr_in));

}
Int32 UDPSocket::ReceiveFrom(char* buffer, size_t size, sockaddr_in* addr)
{
	Int32 flags = 0;
	//Int32 bytesLeft = size;

	/*do
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
	} while (bytesLeft);*/
	socklen_t addrlen = sizeof(sockaddr_in);

	return recvfrom(mSocket, buffer, size, flags, (sockaddr*)addr, &addrlen);
}

Int32 UDPSocket::SendTo(Packet& packet, sockaddr_in* addr)
{
	Int32 packetSize = packet.mData.size();
	Int32 opResult;
	Packet initPacket;
	initPacket << packetSize;

	opResult = SendTo(&initPacket.mData[0], initPacket.mData.size() * sizeof(char), addr);
	if (opResult <= 0)
		return opResult;
	opResult = SendTo(&packet.mData[0], packet.mData.size() * sizeof(char), addr);
	if (opResult <= 0)
		return opResult;

	//std::cout << opResult << " bytes sent.\n";

	return opResult;
}
Int32 UDPSocket::ReceiveFrom(Packet& packet, sockaddr_in* addr)
{
	Int32 packetSize;
	Int32 opResult;
	Packet initPacket;

	initPacket.mData.resize(sizeof(Int32));
	opResult = ReceiveFrom(&initPacket.mData[0], sizeof(Int32), addr);
	if (opResult <= 0)
		return opResult;

	initPacket >> packetSize;

	packet.Clear();
	packet.mData.resize(packetSize);
	opResult = ReceiveFrom(&packet.mData[0], packetSize, addr);
	if (opResult <= 0)
		return opResult;

	//std::cout << opResult << " bytes received.\n";

	return opResult;
}

Int32 UDPSocket::Close()
{
	//freeaddrinfo(mAddrInfo);
	Int32 retval = (mSocket == INVALID_SOCKET) ? 0 : CLOSE_SOCKET(mSocket);
	mSocket = INVALID_SOCKET;
	return retval;
}
