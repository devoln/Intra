#pragma once

#include <Intra/Platform/Error.h>
#include "IntraX/Container/Sequential/String.h"

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED

#ifdef _WIN32
#define INTRAZ_D_CONST_(name, linux, freebsd, apple, win) win
#elif defined(__linux__)
#define INTRAZ_D_CONST_(name, linux, freebsd, apple, win) linux
#elif defined(__FreeBSD__)
#define INTRAZ_D_CONST_(name, linux, freebsd, apple, win) freebsd
#elif defined(__APPLE__)
#define INTRAZ_D_CONST_(name, linux, freebsd, apple, win) apple
#else
#define INTRAZ_D_CONST_(name, linux, freebsd, apple, win) name
#endif

//#define INTRA_DEBUG_SOCKET_CONSTS // define to check the codes against <errno.h>
#ifdef INTRA_DEBUG_SOCKET_CONSTS
#ifdef _WIN32
#include <WinSock2.h>
#define AF_BLUETOOTH AF_BTH
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#if !defined(TCP_KEEPIDLE) && defined(TCP_KEEPALIVE)
#define TCP_KEEPIDLE TCP_KEEPALIVE
#endif

#define INTRAZ_D_CONST_(name, linux, bsd, win) []{constexpr auto res = INTRAZ_D_CONST_(name, linux, freebsd, apple, win); \
	static_assert(name && res == name); return res;}()
#else
#define INTRAZ_D_CONST INTRAZ_D_CONST_
#endif

enum class AddressFamily: uint8
{
	Any = INTRAZ_D_CONST(AF_UNSPEC, 0, 0, 0, 0),
	Local = INTRAZ_D_CONST(AF_UNIX, 1, 1, 1, 1),
	IPv4 = INTRAZ_D_CONST(AF_INET, 2, 2, 2, 2),
	IPv6 = INTRAZ_D_CONST(AF_INET6, 10, 28, 30, 23)
#if defined(_WIN32) || defined(__linux__) || defined(__FreeBSD__) || defined(AF_BLUETOOTH)
	, Bluetooth = INTRAZ_D_CONST(AF_BLUETOOTH, 31, 36, 0, 32)
#if defined(__linux__) || defined(AF_CAN)
	, CAN = INTRAZ_D_CONST(AF_CAN, 29, 0, 0)
#endif
};
enum class SocketType: uint8 {Stream = 1, Datagram, Raw, ReliableDatagram, SeqPacket};
enum class ProtocolType: uint8 {TCP = 6, UDP = 17, ICMP = 1, IGMP = 2};
enum class ShutdownEnd: uint8 {Read, Write, Both};
enum class SockOptLevel {
	Socket = INTRAZ_D_CONST(SOL_SOCKET, 1, 0xFFFF, 0xFFFF, 0xFFFF),
	TCP = INTRAZ_D_CONST(SOL_TCP, 6, 6, 6, 6)
};
enum class SockOptName {
	NoDelay = INTRAZ_D_CONST(TCP_NODELAY, 1, 1, 1, 1),
	ReuseAddr = INTRAZ_D_CONST(SO_REUSEADDR, 2, 4, 4, 4),
	KeepAlive = INTRAZ_D_CONST(SO_KEEPALIVE, 9, 8, 8, 8),
	KeepIdle = INTRAZ_D_CONST(TCP_KEEPIDLE, 4, 3, 0x10, 3),
	KeepInterval = INTRAZ_D_CONST(TCP_KEEPINTVL, 5, 5, 0x101, 17),
	KeepCount = INTRAZ_D_CONST(TCP_KEEPCNT, 6, 6, 0x102, 16),
	FastOpen = INTRAZ_D_CONST(TCP_FASTOPEN, 23, 1025, 0x105, 15)
};

#pragma pack(push, 1)
struct SocketAddr // binary layout of this struct matches largest possible sockaddr struct
{
#if !(defined(__linux__) || defined(_WIN32))
	uint8: 8; // sun_len: supposed to contain value of SizeOf() but seems redundant and unused (is there any OS that needs it?)
#endif
	AddressFamily Family;
#if defined(__linux__) || defined(_WIN32)
	uint8: 8;
#endif
	union
	{
		struct {
#if defined(__linux__) || defined(_WIN32)
			char Name[108]; // File system path or an abstract name (if Name[0] == '\0')
#else
			char Name[104]; // File system path
#endif
		} Unix;
		struct {uint16BE Port; uint8 Addr[4];} IPv4;
		struct {uint16BE Port; uint32BE FlowInfo; uint8 Addr[16]; uint32 ScopeID;} IPv6;
	};

	[[nodiscard]] constexpr size_t SizeOf() const
	{
		return 2 + (
			Family == AddressFamily::IPv4? sizeof(IPv4):
			Family == AddressFamily::IPv6? sizeof(IPv6):
			sizeof(Unix)
		);
	}
	[[nodiscard]] static constexpr SocketAddr MakeIPv4(Array<uint8, 4> ipv4Addr, uint16 port)
	{
		return {
			.Family == AddressFamily::IPv4, .Port = port,
			.Addr = {ipv4Addr[0], ipv4Addr[1], ipv4Addr[2], ipv4Addr[3]}
		};
	}

	[[nodiscard]] static SocketAddr ParseIPv4(ZStringView ipv4Addr, uint16 port);
	[[nodiscard]] static SocketAddr ParseIPv6(ZStringView ipv6Addr, uint16 port);
	[[nodiscard]] static SocketAddr ParseLocal(StringView path);

	// Try parse addrStr as IPv4, then IPv6, otherwise, if allowLocal is true, treat it as Unix domain socket path
	[[nodiscard]] static SocketAddr Parse(StringView addrStr, bool allowLocal = false);

	[[nodiscard]] String ToHostString() const;
	[[nodiscard]] String ToString() const;

	[[nodiscard]] constexpr explicit operator bool() const {return Family != AddressFamily(0);}
};
#pragma pack(pop)
#if defined(_WIN32) || defined(__linux__)
static_assert(sizeof(SocketAddr) == 110);
#endif

class AddrInfo
{
public:
	AddrInfo() = default;
	AddrInfo Create(StringView addr, bool bindAddr, AddressFamily family = AddressFamily::Any);

	struct Node
	{
		int Flags;
		union {AddressFamily Family; int: 32;};
		union {SocketType SockType; int: 32;};
		union {ProtocolType Protocol; int: 32;};
		int AddrLen;
		char* CanonName;
		SocketAddr& Value;
		Node* Next;

		[[nodiscard]] INTRA_FORCEINLINE Node* NextListNode() const {return Next;}
	};
	static_assert(sizeof(void*) == 8? sizeof(Node) == 48: sizeof(Node) == 32, "Node must be binary compatible with struct addrinfo");

	auto ToRange() const {return LinkedRange<SocketAddr, Node>(mFirstInfo.get());}
	explicit operator bool() const {return bool(mFirstInfo);}
private:
	static void freeAi(Node* ai) {z_D::freeaddrinfo(reinterpret_cast<addrinfo*>(ai));}
	ResourceOwner<Node*, TCall<&AddrInfo::freeAi>> mFirstInfo;
};

class Socket
{
public:
	Socket(Socket&&) = default;
	Socket& operator=(Socket&&) = default;
	~Socket() {Close();}

	int Handle() const {return mHandle;}
	Socket FromHandle(int handle) {Socket res; res.mHandle = handle; return handle;}

	void Close();

	ErrorCode SetNonBlocking(bool nonBlocking = true);
	ErrorCode SetNoDelay(bool noDelay = true);
	
	explicit operator bool const {return !mHandle.IsNull();}

protected:
	HandleMovableWrapper<int, -1> mHandle;

	Socket() = default;
	ErrorCode init(AddressFamily, SocketType, ProtocolType);

	void initContext();

	bool waitInputMs(size_t milliseconds) const;
	bool waitInput() const;
	ErrorCode setOpt(SockOptLevel level, SockOptName opt, int value);
};

class TcpConnection: public Socket
{
	friend class TcpListener;

	ErrorCode tryConnect(const SocketAddr& address);
public:
	TcpConnection() = default;
	TcpConnection(TcpConnection&&) = default;
	TcpConnection& operator=(TcpConnection&&) = default;

	static Result<TcpConnection> Connect(const SocketAddr& address);
	static Result<TcpConnection> Connect(const AddrInfo& addresses);
	static Result<TcpConnection> Connect(StringView address) {return Connect(AddrInfo::Create(address, false));}

	static Result<Tuple<TcpConnection, size_t>> ConnectAndWriteSome(const SocketAddr& address, Span<const char> src);

	bool WaitForInputMs(size_t timeoutMs) const {return waitInputMs(timeoutMs);}
	bool WaitForInput() const {return waitInput();}
	bool ReadyToRead() const {return waitInputMs(0);}

	ErrorCode Shutdown(ShutdownEnd end);
	ErrorCode SetTimeout(TimeDelta timeout);

	INTRA_FORCEINLINE LResult<size_t> PeekSome(Span<char> dst) {return receive(dst, 2);} // MSG_PEEK
	INTRA_FORCEINLINE LResult<size_t> ReadSome(Span<char> dst) {return receive(dst, 0);}
	INTRA_FORCEINLINE LResult<size_t> WriteSome(Span<const char> src) {return send(src, 0);}

private:
	LResult<size_t> receive(Span<char> dst, int flags);
	LResult<size_t> send(Span<const char> src, int flags);
};

class TcpListener: public Socket
{
public:
	TcpListener() = default;
	TcpListener(TcpListener&& rhs) = default;
	TcpListener& operator=(TcpListener&& rhs) = default;
	static Result<TcpListener> Bind(const SocketAddr& address, bool allowFastopen = true, int maxListenersLimit = 0);
	static Result<TcpListener> Bind(const AddrInfo& addresses, bool allowFastopen = true, int maxListenersLimit = 0);
	static Result<TcpListener> Bind(StringView address) {return Connect(AddrInfo::Create(address, true));}

	bool WaitForConnectionMs(size_t milliseconds) const {return waitInputMs(milliseconds);}
	bool WaitForConnection() const {return waitInput();}
	bool HasConnections() const {return waitInputMs(0);}

	Result<Tuple<TcpConnection, SocketAddr>> Accept();
};
} INTRA_END
