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

using bufsize_t = int;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
using bufsize_t = size_t;
inline void closesocket(int socket) {close(socket);}
#endif

namespace Intra { INTRA_BEGIN
namespace z_D {
#ifdef _WIN32
struct WsaContext
{
	WSADATA wsaData;
	WsaContext() {z_D::WSAStartup(2, &wsaData);}
	~WsaContext() {z_D::WSACleanup();}
};

inline ErrorCode lastSocketErrorCode() {return ErrorCode::Wrap(WSAGetLastError());}
#undef gai_strerror
#define gai_strerror gai_strerrorA

#else
inline ErrorCode getLastErrorCode() {return ErrorCode::LastError();}
#endif
}

INTRA_IGNORE_WARNS_MSVC(4548)

SocketAddr SocketAddr::ParseIPv4(ZStringView ipv4Addr, uint16 port)
{
	SocketAddr res = {
		.Family == AddressFamily::IPv4,
		.IPv4 = {.Port = port}
	};
	const auto addr = uint32(inet_addr(ipv4Addr.CStr()));
	if(addr + 1 <= 1) return {};
	*reinterpret_cast<uint32*>(res.IPv4.Addr) = addr;
	return res;
}

SocketAddr SocketAddr::ParseIPv6(ZStringView ipv6Addr, uint16 port)
{
	SocketAddr res = {
		.Family == AddressFamily::IPv6,
		.IPv6 = {.Port = port}
	};
	if(inet_pton(int(AddressFamily::IPv6), ipv6Addr.CStr(), res.IPv6.Addr) != 1) return {};
	return res;
}

SocketAddr SocketAddr::ParseLocal(StringView path)
{
	SocketAddr res = {.Family == AddressFamily::Local};
	if(Length(path) >= Length(res.Unix.Name)) return {}; // Too long path
	path.RawUnicodeUnits()|Copy(res.Unix.Name);
	return res;
}

SocketAddr SocketAddr::Parse(ZStringView addrStr, bool allowUnix = false)
{
	auto addrChars = addrStr.RawUnicodeUnits();
	const bool hasDot = addrChars|Contains('.');
	const bool hasColon = addrChars|Contains(':');
	uint16 port = 0;
	if(hasDot) // probably IPv4 (or Unix path)
	{
		if(hasColon)
		{
			const auto portSuffix = addrChars|TakeLastUntil(':');
			addrChars|PopLastCount(Length(portSuffix) + 1);
			port = Intra::Parse<uint16>(portSuffix);
		}
		auto res = ParseIPv4(String(Unsafe, addrChars), port);
		if(res) return res;
		// not valid IPv4 string, try other families
	}
	if(hasColon) // probably IPv6 (or Windows path)
	{
		const auto portSuffix = addrChars|TakeLastUntil("]:");
		addrChars|PopLastCount(Length(portSuffix) + 2);
		auto res = ParseIPv6(String(Unsafe, addrChars), port);
		if(res) return res;
		// not valid IPv6 string, try other families
	}
	if(allowUnix) return ParseLocal(addrStr);
	return {};
}

String SocketAddr::ToHostString() const
{
	if(Family == AddressFamily::IPv4)
	{
		if(auto res = inet_ntoa(reinterpret_cast<unsigned long*>(IPv4.Addr))) return res;
		return {};
	}
	if(Family == AddressFamily::IPv6)
	{
		char strBuf[64];
		if(auto res = inet_ntop(int(AddressFamily::IPv6), IPv6.Addr, strBuf, sizeof(strBuf)))
			return res;
		return {};
	}
	if(Family == AddressFamily::Local) return Unix.Name[0]? Unix.Name: Unix.Name + 1;
	return {};
}

String SocketAddr::ToString() const
{
	auto res = ToHostString();
	if(res.empty()) return {};
	if(Family == AddressFamily::IPv4 || Family == AddressFamily::IPv6)
	{
		res += ':';
		res += Intra::ToString(uint16(IPv4.Port));
	}
	return res;
}


Result<AddrInfo> AddrInfo::Create(StringView addrStr, bool bindAddr, AddressFamily family)
{
	const z_D::addrinfo hints = {
		.ai_flags = bindAddr? AI_PASSIVE: 0,
		.ai_family = int(family)
	};
	auto addrChars = addrStr.RawUnicodeUnits();
	const bool hasColon = addrChars|Contains(':');
	const char* portStr = nullptr;
	String portSuffix;
	if(hasColon)
	{
		portSuffix = String(Unsafe, addrChars|TakeLastUntil(':'));
		addrChars|PopLastCount(Length(portSuffix) + 1);
	}
	AddrInfo res;
	const auto gaiErr = getaddrinfo(String(addrStr).CStr(), portSuffix.empty()? nullptr: portSuffix.CStr(),
		&hints, reinterpret_cast<z_D::addrinfo**>(&res.mFirstInfo));
	static_assert(sizeof(res.mFirstInfo) == sizeof(void*));
	if(gaiErr) return ErrorCode::WrapGAI(gaiErr);
	return res;
}


void Socket::initContext()
{
#ifdef _WIN32
	INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
	static const z_D::WsaContext context;
#endif
}

ErrorCode Socket::init(AddressFamily family, SocketType type, ProtocolType protocol)
{
	initContext();
	mHandle = int(z_D::socket(int(family), int(type), int(protocol)));
	if(mHandle == -1) return z_D::lastSocketErrorCode();
	return {};
}

void Socket::Close()
{
	if(!*this) return;
	z_D::closesocket(SOCKET(mHandle));
	mHandle = {};
}

ErrorCode Socket::SetNonBlocking(bool nonBlocking = true)
{
#ifdef _WIN32
	unsigned long nonBlock = nonBlocking;
	z_D::ioctlsocket(SOCKET(mHandle), 0x4667E, &nonBlock); // FIONBIO
#else
	const int nonBlock = INTRAZ_D_CONST(O_NONBLOCK, 2048, 4, 4);
	z_D::fcntl(mHandle, 4, nonBlocking? nonBlock: 0); // F_SETFL
#endif
}

ErrorCode Socket::SetNoDelay(bool noDelay = true)
{
	return setOpt(SockOptLevel::TCP, SockOptName::NoDelay, noDelay);
}

bool Socket::waitInputMs(size_t milliseconds) const
{
	if(mHandle.IsNull()) return false;
#ifdef _WIN32 // on Windows this use of select is very efficient and available on XP
	fd_set set;
	set.fd_count = 1;
	set.fd_array[0] = SOCKET(mHandle);
	timeval timeout = {long(milliseconds / 1000), long(milliseconds % 1000 * 1000)};
	return z_D::select(1, &set, nullptr, nullptr, &timeout) > 0;
#else
	pollfd pfd = {.fd = mHandle, .events = 0x300};
	return z_D::poll(&fd, 1, milliseconds);
#endif
}

bool Socket::waitInput() const
{
	if(mHandle.IsNull()) return false;
#ifdef _WIN32 // on Windows this use of select is very efficient and available on XP
	fd_set set;
	set.fd_count = 1;
	set.fd_array[0] = SOCKET(mHandle);
	return z_D::select(1, &set, nullptr, nullptr, nullptr) > 0;
#else
	pollfd pfd = {.fd = mHandle, .events = 0x300};
	return z_D::poll(&fd, 1, -1);
#endif
}

ErrorCode Socket::setOpt(SockOptLevel level, SockOptName opt, int value)
{
	if(z_D::setsockopt(SOCKET(mHandle), int(level), int(opt), &value, sizeof(value)) == -1)
		return z_D::lastSocketErrorCode();
	return {};
}

Result<TcpListener> TcpListener::Bind(const SocketAddr& address, bool allowFastopen, int maxListenersLimit)
{
	TcpListener res;
	INTRA_RETURN_ON_ERROR(res.init(address.Family, SocketType::Stream, ProtocolType::TCP));
	INTRA_RETURN_ON_ERROR(setOpt(SockOptLevel::Socket, SockOptName::ReuseAddr, 1));
	if(z_D::bind(SOCKET(mHandle), reinterpret_cast<const sockaddr*>(address), socklen_t(address.SizeOf())) < 0)
		return lastSocketErrorCode();
	setOpt(SockOptLevel::TCP, SockOptName::TcpFastOpen, TargetIsLinux? 5: 1);
	if(z_D::listen(SOCKET(mHandle), int(maxListenersLimit)) < 0)
		return lastSocketErrorCode();
	return res;
}

Result<Tuple<TcpListener, SocketAddr>> TcpListener::Accept()
{
	if(!*this) return {};
	SocketAddr addr;
	socklen_t addrLen = sizeof(addr);
	TcpConnection res;
	res.mHandle = int(z_D::accept(SOCKET(mHandle), reinterpret_cast<sockaddr*>(&addr), &addrLen));
	if(res.mHandle.IsNull()) return z_D::lastSocketErrorCode();
#ifdef _WIN32
	// Accepted sockets inherit the properties of their listener, including WSAEventSelect, which is undesirable
	z_D::WSAEventSelect(res.mHandle, nullptr, 0);
#endif
	return Tuple(INTRA_MOVE(res), INTRA_MOVE(addr));
}

ErrorCode tryConnect(const SocketAddr& address)
{
	if(z_D::connect(SOCKET(mHandle), reinterpret_cast<const sockaddr*>(&address), socklen_t(address.SizeOf())) != 0)
		return z_D::lastSocketErrorCode();
	return ErrorCode::NoError;
}

Result<TcpConnection> TcpConnection::Connect(const SocketAddr& address)
{
	TcpConnection res;
	res.init(address.Family, SocketType::Stream, ProtocolType::TCP);
	INTRA_RETURN_ON_ERROR(res.tryConnect(address));
	return res;
}

Result<TcpConnection> TcpConnection::Connect(const AddrInfo& addresses)
{
	INTRA_PRECONDITION(addresses);
	TcpConnection res;
	res.init(addresses.ToRange().First().Family, SocketType::Stream, ProtocolType::TCP);
	for(auto&& address: addresses)
		INTRA_RETURN_ON_ERROR(res.tryConnect(address));
	return res;
}

Result<Tuple<TcpConnection, size_t>> TcpConnection::ConnectAndWriteSome(const SocketAddr& address, Span<const char> src)
{
	TcpConnection res;
	res.init(address.Family, SocketType::Stream, ProtocolType::TCP);
	int bytesSent = 0;
#ifdef __APPLE__
	sa_endpoints_t endpoints = {
		.sae_dstaddr = reinterpret_cast<const sockaddr*>(&address),
		.sae_dstaddrlen = socklen_t(address.SizeOf())
	};
	if(connectx(socket, &endpoints, SAE_ASSOCID_ANY, CONNECT_DATA_IDEMPOTENT|CONNECT_RESUME_ON_READ_WRITE, nullptr, 0, nullptr, nullptr) != 0)
		return z_D::lastSocketErrorCode();
#elif INTRA_TARGET_IS_BSD || defined(__linux__)
#ifndef __linux__
	INTRA_RETURN_ON_ERROR(setOpt(SockOptLevel::TCP, SockOptName::FastOpen, 1));
#endif
	bytesSent = z_D::sendto(res.mHandle, src.Begin, sockbuflen_t(Length(src)), TargetIsLinux? 0x20000000: 0, //MSG_FASTOPEN
		reinterpret_cast<const sockaddr*>(&address), socklen_t(address.SizeOf()));
	if(bytesSent < 0) return z_D::lastSocketErrorCode();
#elif 0 //defined(_WIN32) // Win 10+ or Vista+?
	INTRA_RETURN_ON_ERROR(setOpt(SockOptLevel::TCP, SockOptName::FastOpen, 1));
	static z_D::LPFN_CONNECTEX connectEx = nullptr;
	if(!connectEx)
	{
		uint32 guid[4] = {0x25a207b9, 0x4660ddf3, 0xe576e98e, 0x3e06748c}; // WSAID_CONNECTEX
		unsigned long numBytes = 0;
		z_D::WSAIoctl(s, 0xC8000006, &guid, sizeof(guid), &connectEx, sizeof(connectEx), &numBytes, nullptr, nullptr); // SIO_GET_EXTENSION_FUNCTION_POINTER
	}
	if(!connectEx(SOCKET(res.mHandle), reinterpret_cast<const sockaddr*>(&address), socklen_t(address.SizeOf()),
		src.Begin, uint32(Length(src)), reinterpret_cast<unsigned long*>(&bytesSent), nullptr))
		return z_D::lastSocketErrorCode();
#else
	INTRA_RETURN_ON_ERROR(res.tryConnect(address));
#endif
	return Tuple(INTRA_MOVE(res), bytesSent);
}

ErrorCode TcpConnection::SetTimeout(TimeDelta timeout)
{
#ifdef _WIN32
	const int vals[] = {1, 1000, Max(200, timeout.IntMilliseconds() / 10) - 100};
	unsigned long bytesReturned;
	if(z_D::WSAIoctl(SOCKET(mHandle), 0x98000004, vals, sizeof(vals), nullptr, 0, &bytesReturned, nullptr, nullptr) != 0) // SIO_KEEPALIVE_VALS
		return z_D::lastSocketErrorCode();
#elif defined(__APPLE__)
	INTRA_RETURN_ON_ERROR(setOpt(SockOptLevel::Socket, SockOptName::KeepAlive, 1));
	INTRA_RETURN_ON_ERROR(setOpt(SockOptLevel::TCP, SockOptName::KeepIdle, int(Ceil(timeout.Seconds()))));
	setOpt(SockOptLevel::TCP, SockOptName(0x20), int(Ceil(timeout.Seconds()))) // TCP_CONNECTIONTIMEOUT
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__)
	int timeoutInSeconds = Max(2, int(Ceil(timeout.Seconds())));
	INTRA_RETURN_ON_ERROR(setOpt(SockOptLevel::Socket, SockOptName::KeepAlive, 1));
	INTRA_RETURN_ON_ERROR(setOpt(SockOptLevel::TCP, SockOptName::KeepIdle, 1));

	int count = 3;
	int interval = (timeoutInSeconds - 1) / (count - 1);
	if(!interval)
	{
		interval = 1;
		count = timeoutInSeconds;
	}
	INTRA_RETURN_ON_ERROR(setOpt(SockOptLevel::TCP, SockOptName::KeepInterval, interval));
	INTRA_RETURN_ON_ERROR(setOpt(SockOptLevel::TCP, SockOptName::KeepCount, count));
#ifdef __linux__
	INTRA_RETURN_ON_ERROR(setOpt(SockOptLevel::TCP, SockOptName(18), count)); // TCP_USER_TIMEOUT
#endif
#endif
}

LResult<size_t> TcpConnection::receive(Span<char> dst, int flags)
{
	const auto res = z_D::recv(SOCKET(mHandle), dst.Begin, bufsize_t(dst.Length()), flags);
	if(res < 0) return z_D::lastSocketErrorCode();
	return size_t(res);
}

LResult<size_t> TcpConnection::send(Span<const char> src, int flags)
{
	const auto res = z_D::send(SOCKET(mHandle), src.Begin, bufsize_t(src.Length()), flags);
	if(res < 0) return z_D::lastSocketErrorCode();
	return size_t(res);
}

ErrorCode TcpConnection::Shutdown(ShutdownEnd end)
{
	if(!mHandle.IsNull() && z_D::shutdown(SOCKET(mHandle), int(end)) != 0)
		return z_D::lastSocketErrorCode();
	return ErrorCode::NoError;
}
} INTRA_END
