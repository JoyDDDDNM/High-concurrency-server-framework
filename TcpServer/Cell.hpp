#ifndef _SERVER_PARA_HPP_
#define _SERVER_PARA_HPP_

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _WIN32				
// in Unix-like environment, the maximum size of fd_set is 1024
// in windows environment, the maximum size of fd_set is 64, in WinSock2.h
// to ensure the consitentcy for cross-platform development, 
// we can increase its size by refining this macro 
#	define FD_SETSIZE 1024 
#   define WIN32_LEAN_AND_MEAN // macro to avoid including duplicate macro when include <windows.h> and <WinSock2.h>
#include <windows.h>  // windows system api
#include <WinSock2.h> // windows socket api 
#else
#   include <unistd.h> // unix standard system interface
#   include <arpa/inet.h>
#	include <string.h>
#   define SOCKET int
#   define INVALID_SOCKET  (SOCKET)(~0)
#   define SOCKET_ERROR            (-1)
#endif

#ifndef RECV_BUFF_SIZE 
// base unit of buffer size
#define RECV_BUFF_SIZE 10240 * 5
#define SEND_BUFF_SIZE RECV_BUFF_SIZE
#endif

#endif