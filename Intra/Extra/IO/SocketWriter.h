#pragma once

#include "Socket.h"

#include "Core/Range/Span.h"

#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Stream/ToString.h"
#include "Core/Range/Stream/OutputStreamMixin.h"

#include "Container/Sequential/Array.h"

INTRA_BEGIN
//! Buffered socket output stream
class SocketWriter: public OutputStreamMixin<SocketWriter, char>
{
public:
	forceinline SocketWriter(null_t=null) {}

	forceinline SocketWriter(StreamSocket&& socket, size_t bufferSize = 4096):
		mSocket(Move(socket))
	{
		if(mSocket == null) return;
		mBuffer.SetCountUninitialized(bufferSize);
		mBufferRest = mBuffer;
	}

	SocketWriter(const SocketWriter&) = delete;
	SocketWriter(SocketWriter&& rhs) noexcept {operator=(Move(rhs));}

	forceinline bool operator==(null_t) {return mSocket == null;}
	forceinline bool operator!=(null_t) {return !operator==(null);}

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

	SocketWriter& operator=(null_t)
	{
		Flush();
		mSocket = null;
		mBuffer = null;
		mBufferRest = null;
		return *this;
	}

	forceinline ~SocketWriter() {Flush();}

	INTRA_NODISCARD forceinline bool Full() const {return mBufferRest.Empty();}
	
	forceinline void Put(char c)
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

	//! Записать буфер в файл.
	void Flush()
	{
		if(mBufferRest.Length() == mBuffer.Length()) return;
		mSocket.Write(mBuffer.Data(), mBuffer.Length() - mBufferRest.Length(), IgnoreErrors);
		mBufferRest = mBuffer;
	}

	INTRA_NODISCARD forceinline StreamSocket& Socket() {return mSocket;}
	INTRA_NODISCARD forceinline const StreamSocket& Socket() const {return mSocket;}

private:
	StreamSocket mSocket;
	Array<char> mBuffer;
	Span<char> mBufferRest;
};
INTRA_END
