#include "Socket.h"
#include "Std.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif
INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <ws2bth.h>
INTRA_WARNING_POP

typedef int bufsize_t;
typedef intptr_t ssize_t;

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

typedef size_t bufsize_t;

inline void closesocket(int socket) {close(socket);}

#endif

INTRA_BEGIN
#ifdef _WIN32
struct WsaContext
{
	WSADATA wsaData;
	bool inited;
	
	WsaContext()
	{
		inited = WSAStartup(MAKEWORD(2, 0), &wsaData) == 0;
	}

	~WsaContext()
	{
		if(inited) WSACleanup();
	}
};

static uint32 getLastErrorCode() {return uint32(HRESULT_FROM_WIN32(static_cast<unsigned long>(WSAGetLastError())));}

static Intra::String getErrorMessage(uint32 err)
{
	char* s = null;
	FormatMessageA(DWORD(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS),
		null, DWORD(err),
		DWORD(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)),
		reinterpret_cast<char*>(&s), 0, null);
	Intra::String errorMsg = Intra::String(s);
	LocalFree(s);
	return errorMsg;
}

#undef gai_strerror
#define gai_strerror gai_strerrorA

#else

int getLastErrorCode() {return 0x80000000|errno;}

inline Intra::Container::String getErrorMessage(int err)
{return Intra::Container::String(strerror(err & (~0x80000000)));}

enum: ssize_t {SOCKET_ERROR = -1};

#endif

#ifdef _MSC_VER
#pragma warning(disable: 4548)
#endif

static const byte socketConstants[][3] =
{
	//{AF_UNIX, SOCK_STREAM, IPPROTO_TCP},
	{AF_INET, SOCK_STREAM, IPPROTO_TCP},
	{AF_INET6, SOCK_STREAM, IPPROTO_TCP},
	{AF_INET, SOCK_DGRAM, IPPROTO_UDP},
	{AF_INET6, SOCK_DGRAM, IPPROTO_UDP},
	{AF_INET, SOCK_RAW, IPPROTO_ICMP},
	{AF_INET6, SOCK_RAW, IPPROTO_IGMP},
	{AF_INET, SOCK_RAW, IPPROTO_ICMP},
	{AF_INET6, SOCK_RAW, IPPROTO_IGMP},
#ifdef _WIN32
	{AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM},
#else
	{0, 0, 0},
#endif
	{AF_INET, SOCK_RAW, IPPROTO_ICMPV6},
	{AF_INET6, SOCK_RAW, IPPROTO_ICMPV6},
#ifdef AF_IRDA
	{AF_IRDA, SOCK_STREAM, 0}
#else
	{0, 0, 0}
#endif
};

void BasicSocket::initContext()
{
#ifdef _WIN32
	INTRA_IGNORE_WARNING_GLOBAL_CONSTRUCTION
	static const WsaContext context;
#endif
}

BasicSocket::BasicSocket(SocketType type, ErrorReporter err): mType(type)
{
	initContext();
	mHandle = socket(
		socketConstants[byte(type)][0],
		socketConstants[byte(type)][1],
		socketConstants[byte(type)][2]);
	if(mHandle == NullSocketHandle)
	{
		const auto errc = getLastErrorCode();
		err.Error({errc}, getErrorMessage(errc), INTRA_SOURCE_INFO);
	}
}

void BasicSocket::Close()
{
	if(mHandle == NullSocketHandle) return;
	closesocket(mHandle);
	mHandle = NullSocketHandle;
}


ServerSocket::ServerSocket(SocketType type, uint16 port, size_t maxConnections, ErrorReporter err)
{
	initContext();
	const addrinfo hints = {AI_PASSIVE,
		socketConstants[byte(type)][0],
		socketConstants[byte(type)][1],
		socketConstants[byte(type)][2],
		0, null, null, null};
	addrinfo* addrInfo = null;
	char portStr[6];
	Span<char>(portStr) << port << '\0';
	const auto gaiErr = getaddrinfo(null, portStr, &hints, &addrInfo);
	if(gaiErr != 0)
	{
		err.Error({uint32(gaiErr)}, StringView("getaddrinfo failed: ") + StringView(gai_strerror(gaiErr)), INTRA_SOURCE_INFO);
		return;
	}
	INTRA_FINALLY{freeaddrinfo(addrInfo);};

	mHandle = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
	if(mHandle == NullSocketHandle)
	{
		const auto errc = getLastErrorCode();
		err.Error({errc}, "Socket creation falied: " + getErrorMessage(errc), INTRA_SOURCE_INFO);
		return;
	}

	int reuseAddrEnable = 1;
	setsockopt(mHandle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&reuseAddrEnable), sizeof(int));

	if(bind(mHandle, addrInfo->ai_addr, socklen_t(addrInfo->ai_addrlen)) != 0)
	{
		const auto errc = getLastErrorCode();
		err.Error({errc}, "Socket bind failed: " + getErrorMessage(errc), INTRA_SOURCE_INFO);
		Close();
		return;
	}

	if(listen(mHandle, int(maxConnections)) < 0)
	{
		const auto errc = getLastErrorCode();
		err.Error({errc}, "Socket listen failed: " + getErrorMessage(errc), INTRA_SOURCE_INFO);
		Close();
		return;
	}
}

bool BasicSocket::waitInputMs(size_t milliseconds) const
{
	fd_set set;
	FD_ZERO(&set);
	FD_SET(mHandle, &set);
	timeval timeout = {long(milliseconds/1000), long(milliseconds % 1000 * 1000)};
	return select(int(mHandle + 1), &set, null, null, &timeout) > 0;
}

bool BasicSocket::waitInput() const
{
	if(mHandle == NullSocketHandle) return false;
	fd_set set;
	FD_ZERO(&set);
	FD_SET(mHandle, &set);
	return select(int(mHandle + 1), &set, null, null, null) > 0;
}

StreamSocket ServerSocket::Accept(String& addr, ErrorReporter err)
{
	if(mHandle == NullSocketHandle) return null;
	sockaddr sAddr;
	socklen_t sAddrLen = sizeof(sAddr);
	StreamSocket result;
	result.mHandle = accept(mHandle, &sAddr, &sAddrLen);
	if(result.mHandle == NullSocketHandle)
	{
		const auto errc = getLastErrorCode();
		err.Error({errc}, getErrorMessage(errc), INTRA_SOURCE_INFO);
		return result;
	}
	result.mType = mType;
	addr = StringView(static_cast<const char*>(sAddr.sa_data));
	return result;
}

StreamSocket::StreamSocket(SocketType type, StringView host, uint16 port, ErrorReporter err):
	BasicSocket(type, err)
{
	const addrinfo hints = {0,
		socketConstants[byte(type)][0],
		socketConstants[byte(type)][1],
		socketConstants[byte(type)][2],
		0, null, null, null};
	addrinfo* addrInfo = null;
	char portStr[6];
	Span<char>(portStr) << port << '\0';
	const auto gaiErr = getaddrinfo(String(host).CStr(), portStr, &hints, &addrInfo);
	if(gaiErr != 0)
	{
		err.Error({uint32(gaiErr)}, StringView(gai_strerror(gaiErr)), INTRA_SOURCE_INFO);
		Close();
		return;
	}

	if(connect(mHandle, addrInfo->ai_addr, socklen_t(addrInfo->ai_addrlen)) < 0)
	{
		const auto errc = getLastErrorCode();
		err.Error({errc}, getErrorMessage(errc).View(), INTRA_SOURCE_INFO);
		Close();
		return;
	}
}

size_t StreamSocket::Receive(void* dst, size_t bytes, ErrorReporter err)
{
	const ssize_t result = recv(mHandle, static_cast<char*>(dst), bufsize_t(bytes), 0);
	if(result == SOCKET_ERROR)
	{
		const auto errc = getLastErrorCode();
		err.Error({errc}, getErrorMessage(errc), INTRA_SOURCE_INFO);
		return 0;
	}
	return size_t(result);
}

size_t StreamSocket::Send(const void* src, size_t bytes, ErrorReporter err)
{
	ssize_t result = send(mHandle, static_cast<const char*>(src), bufsize_t(bytes), 0);
	if(result == SOCKET_ERROR)
	{
		const auto errc = getLastErrorCode();
		err.Error({errc}, getErrorMessage(errc), INTRA_SOURCE_INFO);
		return 0;
	}
	return size_t(result);
}

size_t StreamSocket::Read(void* dst, size_t bytes, ErrorReporter err)
{
	ssize_t result = recv(mHandle, static_cast<char*>(dst), bufsize_t(bytes), MSG_WAITALL);
	if(result == SOCKET_ERROR)
	{
		const auto errc = getLastErrorCode();
		err.Error({errc}, getErrorMessage(errc), INTRA_SOURCE_INFO);
		return 0;
	}
	return size_t(result);
}

size_t StreamSocket::Write(const void* src, size_t bytes, ErrorReporter err) {return Send(src, bytes, err);}

void StreamSocket::ShutdownReading()
{
	if(mHandle == NullSocketHandle) return;
	shutdown(mHandle, 0);
}

void StreamSocket::ShutdownWriting()
{
	if(mHandle == NullSocketHandle) return;
	shutdown(mHandle, 1);
}

void StreamSocket::Shutdown()
{
	if(mHandle == NullSocketHandle) return; 
	shutdown(mHandle, 2);
}
INTRA_END
