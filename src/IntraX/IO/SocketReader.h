#pragma once

#include "Socket.h"

#include "Intra/Core.h"


#include "Intra/Range/Span.h"

#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Stream/Parse.h"
#include "Intra/Range/Stream/InputStreamMixin.h"

#include "IntraX/Container/Sequential/Array.h"

namespace Intra { INTRA_BEGIN
/// �������������� ������ ����� ��� ������ �� ������
class SocketReader: public InputStreamMixin<SocketReader, char>
{
public:
	INTRA_FORCEINLINE SocketReader(decltype(nullptr)=nullptr) {}

	INTRA_FORCEINLINE SocketReader(StreamSocket&& socket, size_t bufferSize=4096):
		mSocket(Move(socket))
	{
		mBuffer.SetCountUninitialized(bufferSize);
		loadBuffer();
	}

	SocketReader(const SocketReader& rhs) = delete;
	SocketReader& operator=(const SocketReader& rhs) = delete;

	INTRA_FORCEINLINE SocketReader(SocketReader&& rhs) {operator=(Move(rhs));}

	SocketReader& operator=(SocketReader&& rhs)
	{
		mSocket = Move(rhs.mSocket);
		mBuffer = Move(rhs.mBuffer);
		mBufferRest = rhs.mBufferRest;
		rhs.mBufferRest = nullptr;
		return *this;
	}

	SocketReader& operator=(decltype(nullptr))
	{
		mSocket = nullptr;
		mBuffer = nullptr;
		mBufferRest = nullptr;
		return *this;
	}

	INTRA_FORCEINLINE char First() const {return mBufferRest.First();}
	
	INTRA_FORCEINLINE bool Empty() const {return mBufferRest.Empty();}
	
	void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		mBufferRest.PopFirst();
		if(!mBufferRest.Empty()) return;
		loadBuffer();
	}

	size_t PopFirstCount(size_t maxToPop)
	{
		size_t bytesLeft = maxToPop;
		while(!Empty())
		{
			bytesLeft -= mBufferRest.PopFirstCount(bytesLeft);
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

	INTRA_FORCEINLINE Span<const char> BufferedData() const {return mBufferRest;}

	INTRA_FORCEINLINE StreamSocket& Socket() {return mSocket;}
	INTRA_FORCEINLINE const StreamSocket& Socket() const {return mSocket;}

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
} INTRA_END
