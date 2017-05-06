#pragma once

#include "Socket.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Container/Sequential/Array.h"
#include "Range/Mutation/Copy.h"
#include "Utils/Span.h"
#include "Range/Stream/Operators.h"
#include "Range/Stream/OutputStreamMixin.h"


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

//! Буферизованный поток вывода для записи в потоковый сокет.
class SocketWriter: public Range::OutputStreamMixin<SocketWriter, char>
{
public:
	forceinline SocketWriter(null_t=null) {}

	forceinline SocketWriter(StreamSocket&& socket, size_t bufferSize=4096):
		mSocket(Cpp::Move(socket))
	{
		if(mSocket == null) return;
		mBuffer.SetCountUninitialized(bufferSize);
		mBufferRest = mBuffer;
	}

	SocketWriter(const SocketWriter&) = delete;
	SocketWriter(SocketWriter&& rhs) {operator=(Cpp::Move(rhs));}

	forceinline bool operator==(null_t) {return mSocket == null;}
	forceinline bool operator!=(null_t) {return !operator==(null);}

	SocketWriter& operator=(const SocketWriter& rhs) = delete;

	SocketWriter& operator=(SocketWriter&& rhs)
	{
		Flush();
		mSocket = Cpp::Move(rhs.mSocket);
		mBuffer = Cpp::Move(rhs.mBuffer);
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

	forceinline bool Empty() const {return mBufferRest.Empty();}
	
	forceinline void Put(char c)
	{
		INTRA_DEBUG_ASSERT(!Empty());
		mBufferRest.Put(c);
		if(mBufferRest.Empty()) Flush();
	}

	size_t PutAllAdvance(CSpan<char>& src)
	{
		size_t totalBytesWritten = Range::CopyAdvanceToAdvance(src, mBufferRest);
		if(!mBufferRest.Empty()) return totalBytesWritten;

		Flush();
		if(src.Length() >= mBuffer.Length())
		{
			const size_t bytesWritten = mSocket.Write(src.Data(), src.Length());
			src.Begin += bytesWritten;
			totalBytesWritten += bytesWritten;
		}
		totalBytesWritten += Range::CopyAdvanceToAdvance(src, mBufferRest);
		return totalBytesWritten;
	}

	template<typename AR> Meta::EnableIf<
		Range::IsArrayRangeOfExactly<AR, char>::_ && !Meta::IsConst<AR>::_,
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
		mSocket.Write(mBuffer.Data(), mBuffer.Length()-mBufferRest.Length());
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
