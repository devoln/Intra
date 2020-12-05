#pragma once

#include "Socket.h"

#include "Intra/Range/Span.h"

#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Stream/ToString.h"
#include "Intra/Range/Stream/OutputStreamMixin.h"

#include "IntraX/Container/Sequential/Array.h"

INTRA_BEGIN
/// Buffered socket output stream
class SocketWriter: public OutputStreamMixin<SocketWriter, char>
{
public:
	INTRA_FORCEINLINE SocketWriter(decltype(null)=null) {}

	INTRA_FORCEINLINE SocketWriter(StreamSocket&& socket, size_t bufferSize = 4096):
		mSocket(Move(socket))
	{
		if(mSocket == null) return;
		mBuffer.SetCountUninitialized(bufferSize);
		mBufferRest = mBuffer;
	}

	SocketWriter(const SocketWriter&) = delete;
	SocketWriter(SocketWriter&& rhs) noexcept {operator=(Move(rhs));}

	INTRA_FORCEINLINE bool operator==(decltype(null)) {return mSocket == null;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) {return !operator==(null);}

	SocketWriter& operator=(const SocketWriter& rhs) = delete;

	SocketWriter& operator=(SocketWriter&& rhs) noexcept
	{
		Flush();
		mSocket = Move(rhs.mSocket);
		mBuffer = Move(rhs.mBuffer);
		mBufferRest = rhs.mBufferRest;
		rhs.mBufferRest = null;
		return *this;
	}

	SocketWriter& operator=(decltype(null))
	{
		Flush();
		mSocket = null;
		mBuffer = null;
		mBufferRest = null;
		return *this;
	}

	INTRA_FORCEINLINE ~SocketWriter() {Flush();}

	[[nodiscard]] INTRA_FORCEINLINE bool Full() const {return mBufferRest.Empty();}
	
	INTRA_FORCEINLINE void Put(char c)
	{
		INTRA_PRECONDITION(!Full());
		mBufferRest.Put(c);
		if(mBufferRest.Empty()) Flush();
	}

	size_t PutAllAdvance(CSpan<char>& src)
	{
		size_t totalBytesWritten = ReadWrite(src, mBufferRest);
		if(!mBufferRest.Empty()) return totalBytesWritten;

		Flush();
		if(src.Length() >= mBuffer.Length())
		{
			const size_t bytesWritten = mSocket.Write(src.Data(), src.Length(), IgnoreErrors);
			src.Begin += bytesWritten;
			totalBytesWritten += bytesWritten;
		}
		totalBytesWritten += ReadWrite(src, mBufferRest);
		return totalBytesWritten;
	}

	template<typename AR> Requires<
		CArrayRangeOfExactly<AR, char> &&
		!CConst<AR>,
	size_t> PutAllAdvance(AR& src)
	{
		CSpan<char> srcArr(src.Data(), src.Length());
		size_t result = PutAllAdvance(srcArr);
		PopFirstExactly(src, result);
		return result;
	}

	/// �������� ����� � ����.
	void Flush()
	{
		if(mBufferRest.Length() == mBuffer.Length()) return;
		mSocket.Write(mBuffer.Data(), mBuffer.Length() - mBufferRest.Length(), IgnoreErrors);
		mBufferRest = mBuffer;
	}

	[[nodiscard]] INTRA_FORCEINLINE StreamSocket& Socket() {return mSocket;}
	[[nodiscard]] INTRA_FORCEINLINE const StreamSocket& Socket() const {return mSocket;}

private:
	StreamSocket mSocket;
	Array<char> mBuffer;
	Span<char> mBufferRest;
};
INTRA_END
