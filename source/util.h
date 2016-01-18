#ifndef UTIL_H
#define UTIL_H

#include <memory>
#include <iostream>

#include "TypeDefs.h"

#if WIN32

#include <winsock2.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

typedef SOCKET Socket;
typedef DWORD Timeval;

#define CLOSE_SOCKET(a) closesocket(a)
#define GET_TCP_PROTO IPPROTO_TCP
#define GET_UDP_PROTO IPPROTO_UDP

#endif


#if __unix__

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <string.h>

typedef Int32 Socket;
typedef timeval Timeval;

#define GET_TCP_PROTO getprotobyname("TCP")->p_proto
#define GET_UDP_PROTO getprotobyname("UDP")->p_proto
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define CLOSE_SOCKET(a) close(a)
#endif

Int32 SocketStartUp();
Int32 SocketCleanUp();
void sleep(Int32 milliseconds);
void setTimeval(Timeval& tv, Int32 milliseconds);
void getSockaddr(sockaddr_in* sa, const char *addr, Int32 port);

#endif
