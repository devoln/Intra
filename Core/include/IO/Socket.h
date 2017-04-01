#pragma once

#include "Platform/FundamentalTypes.h"
#include "Container/Sequential/String.h"
#include "Platform/PlatformInfo.h"

namespace Intra { namespace IO {

//enum class AddressFamily: byte {Local, IPv4, IPv6, Bluetooth, IrDA};
//enum class SocketType: byte {Stream=1, Datagram, Raw, ReliableDatagram, SeqPacket};
//enum class ProtocolType: byte {TCP, UDP, ICMP, IGMP, TCP, UDP};

enum class SocketType: byte
{
	TCP, TCP_IPv6, UDP, UDP_IPv6,
	ICMP, ICMP_IPv6, IGMP, IGMP_IPv6, BluetoothRFCOMM, ICMPv6, ICMPv6_IPv6, IrDA
};

class BasicSocket
{
protected:
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	enum: uint {NullSocketHandle = ~0u};
public:
	typedef size_t NativeHandle;
#else
	enum: int {NullSocketHandle = -1};
public:
	typedef int NativeHandle;
#endif
public:
	BasicSocket& operator=(BasicSocket&& rhs)
	{
		if(mHandle == rhs.mHandle) return *this;
		Close();
		mHandle = rhs.mHandle;
		mType = rhs.mType;
		rhs.mHandle = NullSocketHandle;
		return *this;
	}

	SocketType Type() const {return mType;}

	NativeHandle GetNativeHandle() const {return mHandle;}

	void Close();
	BasicSocket& operator=(null_t) {Close(); return *this;}
	
	bool operator==(null_t) const {return mHandle == NullSocketHandle;}
	bool operator!=(null_t) const {return !operator==(null);}

protected:
	NativeHandle mHandle;
	SocketType mType;

	BasicSocket(null_t=null): mHandle(NullSocketHandle) {}
	BasicSocket(SocketType type);
	BasicSocket(BasicSocket&& rhs): mHandle(rhs.mHandle), mType(rhs.mType) {rhs.mHandle = NullSocketHandle;}
	~BasicSocket() {Close();}

	void initContext();

	bool waitInputMs(size_t milliseconds) const;
	bool waitInput() const;
};

class StreamSocket: public BasicSocket
{
	friend class ServerSocket;
public:
	StreamSocket(null_t=null) {}
	StreamSocket(SocketType type, StringView host, ushort port);

	bool WaitForInputMs(size_t timeoutMs) const {return waitInputMs(timeoutMs);}
	bool WaitForInput() const {return waitInput();}
	bool ReadyToRead() const {return waitInputMs(0);}

	size_t Read(void* dst, size_t bytes);
	size_t Write(const void* src, size_t bytes);
};

class ServerSocket: public BasicSocket
{
public:
	enum TNonBlocking {NonBlocking};

	ServerSocket(null_t=null) {}
	ServerSocket(SocketType type, ushort port, size_t maxConnections);
	ServerSocket(ServerSocket&& rhs): BasicSocket(Meta::Move(rhs)) {}

	bool WaitForConnectionMs(size_t milliseconds) const {return waitInputMs(milliseconds);}
	bool WaitForConnection() const {return waitInput();}
	bool HasConnections() const {return waitInputMs(0);}

	StreamSocket Accept(String& oAddr);
	StreamSocket Accept() {String addr; return Accept(addr);}
};

}}
