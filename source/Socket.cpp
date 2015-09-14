#include "Socket.h"

#ifdef WIN32
int SocketStartUp()
{
	// Initialize Winsock
	WSADATA wsaData;
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
}
int SocketCleanUp()
{
	return WSACleanup();
}

void sleep(int milliseconds)
{
	Sleep(milliseconds);
}
#endif

#if __unix__
int SocketStartUp()
{
	return 0;
}
int SocketCleanUp()
{
	return 0;
}
void sleep(int milliseconds)
{
	usleep(milliseconds);
}
#endif