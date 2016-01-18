#include "TcpListener.h"

TCPListener::TCPListener()
{
	mSocket = INVALID_SOCKET;
	mAddrInfo = NULL;
}

Int32 TCPListener::Bind(const char* port)
{
	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, port, &hints, &mAddrInfo);
	mSocket = socket(mAddrInfo->ai_family, mAddrInfo->ai_socktype, mAddrInfo->ai_protocol);
	if (mSocket == INVALID_SOCKET)
		return -1;
	
	return bind(mSocket, mAddrInfo->ai_addr, (Int32)mAddrInfo->ai_addrlen);
}

Int32 TCPListener::Listen()
{
	return listen(mSocket, SOMAXCONN);
}
TCPSocket* TCPListener::Accept()
{
	Socket socket = accept(mSocket, NULL, NULL);

	if (socket == INVALID_SOCKET)
		return NULL;

	return new TCPSocket(socket);
}
Int32 TCPListener::Close()
{
	freeaddrinfo(mAddrInfo);
	return CLOSE_SOCKET(mSocket);
}