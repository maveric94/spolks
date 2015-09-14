#ifndef SOCKET_H
#define SOCKET_H

#include <memory>

int SocketStartUp();
int SocketCleanUp();
void sleep(int milliseconds);


#if WIN32

#include <winsock2.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

typedef SOCKET Socket;

#define CLOSE_SOCKET(a) closesocket(a)
#define GET_TCP_PROTO IPPROTO_TCP

#endif


#if __unix__

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

typedef int Socket;

#define GET_TCP_PROTO getprotobyname("TCP")->p_proto
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define CLOSE_SOCKET(a) close(a)



#endif

#endif
