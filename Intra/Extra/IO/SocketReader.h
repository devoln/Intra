#pragma once

#include "Socket.h"

#include "Core/Core.h"


#include "Core/Range/Span.h"

#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Stream/Parse.h"
#include "Core/Range/Stream/InputStreamMixin.h"

#include "Container/Sequential/Array.h"

INTRA_BEGIN
//! �������������� ������ ����� ��� ������ �� ������
class SocketReader: public InputStreamMixin<SocketReader, char>
{
public:
	forceinline SocketReader(null_t=null) {}

	forceinline SocketReader(StreamSocket&& socket, size_t bufferSize=4096):
		mSocket(Move(socket))
	{
		mBuffer.SetCountUninitialized(bufferSize);
		loadBuffer();
	}

	SocketReader(const SocketReader& rhs) = delete;
	SocketReader& operator=(const SocketReader& rhs) = delete;

	forceinline SocketReader(SocketReader&& rhs) {operator=(Move(rhs));}

	SocketReader& operator=(SocketReader&& rhs)
	{
		mSocket = Move(rhs.mSocket);
		mBuffer = Move(rhs.mBuffer);
		mBufferRest = rhs.mBufferRest;
		rhs.mBufferRest = null;
		return *this;
	}

	SocketReader& operator=(null_t)
	{
		mSocket = null;
		mBuffer = null;
		mBufferRest = null;
		return *this;
	}

	forceinline char First() const {return mBufferRest.First();}
	
	forceinline bool Empty() const {return mBufferRest.Empty();}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}
	
	void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		mBufferRest.PopFirst();
		if(!mBufferRest.Empty()) return;
		loadBuffer();
	}

	size_t PopFirstN(size_t maxToPop)
	{
		size_t bytesLeft = maxToPop;
		while(!Empty())
		{
			bytesLeft -= mBufferRest.PopFirstN(bytesLeft);
			if(bytesLeft == 0) break;
			loadBuffer();
		}
		return maxToPop - bytesLeft;
	}

	size_t ReadWrite(Span<char>& dst)
	{
		size_t totalBytesRead = Intra::ReadWrite(mBufferRest, dst);
		if(!mBufferRest.Empty()) return totalBytesRead;

		if(dst.Length() >= mBuffer.Length())
		{
			const size_t bytesRead = mSocket.Read(dst.Data(), dst.Length(), IgnoreErrors);
			dst.Begin += bytesRead;
			totalBytesRead += bytesRead;
		}
		loadBuffer();
		totalBytesRead += Intra::ReadWrite(mBufferRest, dst);
		return totalBytesRead;
	}

	template<typename AR> Requires<
		CArrayRangeOfExactly<AR, char> &&
		!CConst<AR>,
	size_t> ReadWrite(AR& dst)
	{
		Span<char> dstArr(dst.Data(), dst.Length());
		const size_t result = ReadWrite(dstArr);
		PopFirstExactly(dst, result);
		return result;
	}

	forceinline CSpan<char> BufferedData() const {return mBufferRest;}

	forceinline StreamSocket& Socket() {return mSocket;}
	forceinline const StreamSocket& Socket() const {return mSocket;}

private:
	void loadBuffer()
	{
		const size_t bytesRead = mSocket.Receive(mBuffer.Data(), mBuffer.Length(), IgnoreErrors);
		mBufferRest = mBuffer.Take(bytesRead);
	}

	StreamSocket mSocket;
	Array<char> mBuffer;
	Span<char> mBufferRest;
};
INTRA_END
