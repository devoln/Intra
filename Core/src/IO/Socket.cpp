#include "IO/Socket.h"
#include "Platform/PlatformInfo.h"
#include "Utils/Finally.h"

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")

#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2bth.h>

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

inline void closesocket(int socket) {close(socket);}

#endif

namespace Intra { namespace IO {

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
struct WsaContext
{
	WSADATA wsaData;
	bool inited;
	
	WsaContext()
	{
		inited = WSAStartup(MAKEWORD(2, 2), &wsaData) != 0;
	}

	~WsaContext()
	{
		if(inited) WSACleanup();
	}
};

#undef gai_strerror
#define gai_strerror gai_strerrorA

#endif

static const byte socketConstants[][3] =
{
	//{AF_UNIX, SOCK_STREAM, IPPROTO_TCP},
	{AF_INET, SOCK_STREAM, IPPROTO_UDP},
	{AF_INET6, SOCK_STREAM, IPPROTO_TCP},
	{AF_INET, SOCK_DGRAM, IPPROTO_UDP},
	{AF_INET6, SOCK_DGRAM, IPPROTO_UDP},
	{AF_INET, SOCK_RAW, IPPROTO_ICMP},
	{AF_INET6, SOCK_RAW, IPPROTO_IGMP},
	{AF_INET, SOCK_RAW, IPPROTO_ICMP},
	{AF_INET6, SOCK_RAW, IPPROTO_IGMP},
	{AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM},
	{AF_INET, SOCK_RAW, IPPROTO_ICMPV6},
	{AF_INET6, SOCK_RAW, IPPROTO_ICMPV6},
	{AF_IRDA, SOCK_STREAM, 0}
};

void BasicSocket::initContext()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	static const WsaContext context;
#endif
}

BasicSocket::BasicSocket(SocketType type): mType(type)
{
	initContext();
	mHandle = socket(
		socketConstants[byte(type)][0],
		socketConstants[byte(type)][1],
		socketConstants[byte(type)][2]);
}

void BasicSocket::Close()
{
	if(mHandle == NullSocketHandle) return;
	closesocket(mHandle);
	mHandle = NullSocketHandle;
}


ServerSocket::ServerSocket(SocketType type, ushort port, size_t maxConnections)
{
	initContext();
	const addrinfo hints = {AI_PASSIVE,
		socketConstants[byte(type)][0],
		socketConstants[byte(type)][1],
		socketConstants[byte(type)][2],
		0, null, null, null};
	addrinfo* addrInfo = null;
	char portStr[6];
	ArrayRange<char>(portStr) << port << '\0';
	const auto gaiErr = getaddrinfo(null, portStr, &hints, &addrInfo);
	if(gaiErr != 0)
	{
		//gai_strerror(gaiErr);
		return;
	}
	auto addrAutoCleanup = Finally(freeaddrinfo, addrInfo);

	if(bind(mHandle, addrInfo->ai_addr, addrInfo->ai_addrlen) != 0)
	{
		return;
	}
	mHandle = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
	addrInfo = null;

	if(mHandle == NullSocketHandle)
	{
		return;
	}
	if(listen(mHandle, int(maxConnections)) < 0)
	{
		Close();
		return;
	}
}

bool ServerSocket::WaitForConnectionMs(size_t milliseconds) const
{
	fd_set set;
	FD_ZERO(&set);
	FD_SET(mHandle, &set);
	const timeval timeout = {milliseconds/1000, milliseconds % 1000 * 1000};
	return select(mHandle+1, &set, null, null, &timeout) > 0;
}

bool ServerSocket::WaitForConnection() const
{
	fd_set set;
	FD_ZERO(&set);
	FD_SET(mHandle, &set);
	return select(mHandle+1, &set, null, null, null) > 0;
}

bool ServerSocket::HasConnections() const
{return WaitForConnectionMs(0);}

StreamSocket ServerSocket::Accept(String& addr)
{
	if(mHandle == NullSocketHandle) return null;
	sockaddr sAddr;
	int sAddrLen;
	StreamSocket result;
	result.mHandle = accept(mHandle, &sAddr, &sAddrLen);
	result.mType = mType;
	addr = StringView(LPCSTR(sAddr.sa_data));
	return result;
}

StreamSocket::StreamSocket(SocketType type, StringView host, ushort port):
	BasicSocket(type)
{
	const addrinfo hints = {0,
		socketConstants[byte(type)][0],
		socketConstants[byte(type)][1],
		socketConstants[byte(type)][2],
		0, null, null, null};
	addrinfo* addrInfo = null;
	char portStr[6];
	ArrayRange<char>(portStr) << port << '\0';
	const auto gaiErr = getaddrinfo(String(host).CStr(), portStr, &hints, &addrInfo);
	if(gaiErr != 0)
	{
		//gai_strerror(gaiErr);
		Close();
		return;
	}

	if(connect(mHandle, addrInfo->ai_addr, int(addrInfo->ai_addrlen)) < 0)
	{
		//strerror(errno);
		Close();
		return;
	}
}

}}

