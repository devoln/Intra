#pragma once

#include "Socket.h"

#include "Core/Core.h"


#include "Core/Range/Span.h"

#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Stream/ToString.h"
#include "Core/Range/Stream/OutputStreamMixin.h"

#include "Container/Sequential/Array.h"


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace IO {

//! Буферизованный поток вывода для записи в потоковый сокет.
class SocketWriter: public Range::OutputStreamMixin<SocketWriter, char>
{
public:
	forceinline SocketWriter(null_t=null) {}

	forceinline SocketWriter(StreamSocket&& socket, size_t bufferSize=4096):
		mSocket(Move(socket))
	{
		if(mSocket == null) return;
		mBuffer.SetCountUninitialized(bufferSize);
		mBufferRest = mBuffer;
	}

	SocketWriter(const SocketWriter&) = delete;
	SocketWriter(SocketWriter&& rhs) {operator=(Move(rhs));}

	forceinline bool operator==(null_t) {return mSocket == null;}
	forceinline bool operator!=(null_t) {return !operator==(null);}

	SocketWriter& operator=(const SocketWriter& rhs) = delete;

	SocketWriter& operator=(SocketWriter&& rhs)
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

	forceinline bool Full() const {return mBufferRest.Empty();}
	
	forceinline void Put(char c)
	{
		INTRA_DEBUG_ASSERT(!Full());
		mBufferRest.Put(c);
		if(mBufferRest.Empty()) Flush();
	}

	size_t PutAllAdvance(CSpan<char>& src)
	{
		size_t totalBytesWritten = Range::ReadWrite(src, mBufferRest);
		if(!mBufferRest.Empty()) return totalBytesWritten;

		Flush();
		if(src.Length() >= mBuffer.Length())
		{
			const size_t bytesWritten = mSocket.Write(src.Data(), src.Length(), Error::Skip());
			src.Begin += bytesWritten;
			totalBytesWritten += bytesWritten;
		}
		totalBytesWritten += Range::ReadWrite(src, mBufferRest);
		return totalBytesWritten;
	}

	template<typename AR> Requires<
		CArrayRangeOfExactly<AR, char>::_ &&
		!CConst<AR>::_,
	size_t> PutAllAdvance(AR& src)
	{
		CSpan<char> srcArr(src.Data(), src.Length());
		size_t result = PutAllAdvance(srcArr);
		Range::PopFirstExactly(src, result);
		return result;
	}

	//! Записать буфер в файл.
	void Flush()
	{
		if(mBufferRest.Length() == mBuffer.Length()) return;
		mSocket.Write(mBuffer.Data(), mBuffer.Length() - mBufferRest.Length(), Error::Skip());
		mBufferRest = mBuffer;
	}

	forceinline StreamSocket& Socket() {return mSocket;}
	forceinline const StreamSocket& Socket() const {return mSocket;}

private:
	StreamSocket mSocket;
	Array<char> mBuffer;
	Span<char> mBufferRest;
};

}}

INTRA_WARNING_POP
