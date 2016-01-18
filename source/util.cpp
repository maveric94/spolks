#include "util.h"

#ifdef WIN32
Int32 SocketStartUp()
{
	// Initialize Winsock
	WSADATA wsaData;
	return WSAStartup(MAKEWORD(2, 2), &wsaData);
}
Int32 SocketCleanUp()
{
	return WSACleanup();
}

void sleep(Int32 milliseconds)
{
	Sleep(milliseconds);
}
void setTimeval(Timeval& tv, Int32 milliseconds)
{
	tv = milliseconds;
}
void getSockaddr(sockaddr_in* sa, const char *addr, Int32 port)
{
	memset((char *)sa, 0, sizeof(sockaddr_in));
	sa->sin_family = AF_INET;
	sa->sin_port = htons(port);
	sa->sin_addr.S_un.S_addr = inet_addr(addr);
}
#endif



#if __unix__
Int32 SocketStartUp()
{
	return 0;
}
Int32 SocketCleanUp()
{
	return 0;
}
void sleep(Int32 milliseconds)
{
	usleep(milliseconds);
}
void setTimeval(Timeval& tv, Int32 milliseconds)
{
	tv.tv_sec = milliseconds / 1000;
	tv.tv_usec = milliseconds % 1000;
}

void getSockaddr(sockaddr_in* sa, const char *addr, Int32 port)
{
	memset((char *) sa, 0, sizeof(sockaddr_in));
	sa->sin_family = AF_INET;
	sa->sin_port = htons(port);

	if (!inet_aton(addr, &sa->sin_addr))
	{
		std::cout << "Error./n";
	}
}
#endif