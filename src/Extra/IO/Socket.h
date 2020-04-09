#pragma once

#include "Extra/System/Error.h"
#include "Extra/Container/Sequential/String.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
//enum class AddressFamily: byte {Local, IPv4, IPv6, Bluetooth, IrDA};
//enum class SocketType: byte {Stream=1, Datagram, Raw, ReliableDatagram, SeqPacket};
//enum class ProtocolType: byte {TCP, UDP, ICMP, IGMP, TCP, UDP};

enum class SocketType: byte
{
	TCP, TCP_IPv6, UDP, UDP_IPv6,
	ICMP, ICMP_IPv6, IGMP, IGMP_IPv6, BluetoothRFCOMM, ICMPv6, ICMPv6_IPv6, IrDA,
	End
};

class BasicSocket
{
protected:
#ifdef _WIN32
	enum: unsigned {NullSocketHandle = ~0u};
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
	BasicSocket& operator=(decltype(null)) {Close(); return *this;}
	
	bool operator==(decltype(null)) const {return mHandle == NullSocketHandle;}
	bool operator!=(decltype(null)) const {return !operator==(null);}

protected:
	NativeHandle mHandle;
	SocketType mType;

	BasicSocket(decltype(null) = null): mHandle(NullSocketHandle), mType(SocketType::End) {}
	BasicSocket(SocketType type, ErrorReporter err);

	BasicSocket(BasicSocket&& rhs):
		mHandle(rhs.mHandle), mType(rhs.mType) {rhs.mHandle = NullSocketHandle;}
	
	~BasicSocket() {Close();}

	void initContext();

	bool waitInputMs(size_t milliseconds) const;
	bool waitInput() const;
};

class StreamSocket: public BasicSocket
{
	friend class ServerSocket;
public:
	StreamSocket(decltype(null)=null) {}
	StreamSocket(SocketType type, StringView host, uint16 port, ErrorReporter err);
	StreamSocket(StreamSocket&& rhs): BasicSocket(Move(rhs)) {}
	~StreamSocket() {Shutdown();}

	StreamSocket& operator=(StreamSocket&& rhs)
	{
		BasicSocket::operator=(Move(rhs));
		return *this;
	}

	StreamSocket& operator=(decltype(null))
	{
		Shutdown();
		BasicSocket::operator=(null);
		return *this;
	}

	bool WaitForInputMs(size_t timeoutMs) const {return waitInputMs(timeoutMs);}
	bool WaitForInput() const {return waitInput();}
	bool ReadyToRead() const {return waitInputMs(0);}

	void ShutdownReading();
	void ShutdownWriting();
	void Shutdown();

	size_t Read(void* dst, size_t bytes, ErrorReporter err);
	size_t Write(const void* src, size_t bytes, ErrorReporter err);
	size_t Receive(void* dst, size_t bytes, ErrorReporter err);
	size_t Send(const void* src, size_t bytes, ErrorReporter err);
};

class ServerSocket: public BasicSocket
{
public:
	enum TNonBlocking {NonBlocking};

	ServerSocket(decltype(null)=null) {}
	ServerSocket(SocketType type, uint16 port, size_t maxConnections, ErrorReporter err);

	ServerSocket(ServerSocket&& rhs): BasicSocket(Move(rhs)) {}

	ServerSocket& operator=(ServerSocket&& rhs)
	{
		BasicSocket::operator=(Move(rhs));
		return *this;
	}

	ServerSocket& operator=(decltype(null))
	{
		BasicSocket::operator=(null);
		return *this;
	}

	bool WaitForConnectionMs(size_t milliseconds) const {return waitInputMs(milliseconds);}
	bool WaitForConnection() const {return waitInput();}
	bool HasConnections() const {return waitInputMs(0);}

	StreamSocket Accept(String& oAddr, ErrorReporter err);
	StreamSocket Accept(ErrorReporter err) {String addr; return Accept(addr, err);}
};
INTRA_END
