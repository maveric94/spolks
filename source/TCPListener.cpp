#include "TcpListener.h"

TCPListener::TCPListener()
{
	mSocket = INVALID_SOCKET;
	mAddrInfo = nullptr;
}

int TCPListener::Bind(const char* port)
{
	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(nullptr, port, &hints, &mAddrInfo);
	mSocket = socket(mAddrInfo->ai_family, mAddrInfo->ai_socktype, mAddrInfo->ai_protocol);
	if (mSocket == INVALID_SOCKET)
		return -1;
	
	return bind(mSocket, mAddrInfo->ai_addr, (int)mAddrInfo->ai_addrlen);
}

int TCPListener::Listen()
{
	return listen(mSocket, SOMAXCONN);
}
TCPSocket* TCPListener::Accept()
{
	Socket socket = accept(mSocket, NULL, NULL);

	if (socket == INVALID_SOCKET)
		return nullptr;

	return new TCPSocket(socket);
}
int TCPListener::Close()
{
	freeaddrinfo(mAddrInfo);
	return CLOSE_SOCKET(mSocket);
}