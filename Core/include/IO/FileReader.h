#pragma once

#include "OsFile.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Container/Sequential/Array.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Stream/Operators.h"
#include "Range/Stream/InputStreamMixin.h"
#include "Memory/SmartRef/Shared.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {


class OsFile;

class FileReader: public Range::InputStreamMixin<FileReader, char>
{
public:
	forceinline FileReader(null_t=null): mOffset(0), mSize(0) {}

	forceinline FileReader(Shared<OsFile> file, size_t bufferSize=4096):
		mFile(Meta::Move(file)), mOffset(0), mSize(mFile==null? 0: mFile->Size())
	{
		mBuffer.SetCountUninitialized(bufferSize);
		loadBuffer();
	}

	forceinline FileReader(const FileReader& rhs) {operator=(rhs);}
	forceinline FileReader(FileReader&& rhs) {operator=(Meta::Move(rhs));}

	FileReader& operator=(const FileReader& rhs)
	{
		mFile = rhs.mFile;
		mOffset = rhs.mOffset;
		mSize = rhs.mSize;
		mBuffer.SetCountUninitialized(rhs.mBuffer.Length());
		mBufferRest = mBuffer(0, rhs.mBufferRest.Length());
		Algo::CopyTo(rhs.mBufferRest, mBufferRest);
		return *this;
	}

	FileReader& operator=(FileReader&& rhs)
	{
		mFile = Meta::Move(rhs.mFile);
		mOffset = rhs.mOffset;
		mSize = rhs.mSize;
		mBuffer = Meta::Move(rhs.mBuffer);
		mBufferRest = rhs.mBufferRest;
		rhs.mBufferRest = null;
		return *this;
	}

	FileReader& operator=(null_t)
	{
		mFile = null;
		mOffset = 0;
		mSize = 0;
		mBuffer = null;
		mBufferRest = null;
		return *this;
	}

	forceinline char First() const {return mBufferRest.First();}
	
	forceinline bool Empty() const {return mBufferRest.Empty();}

	forceinline size_t Length() const {return size_t(mSize+PositionInFile());}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}
	
	void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		mBufferRest.PopFirst();
		if(!mBufferRest.Empty()) return;
		loadBuffer();
	}

	size_t PopFirstN(size_t maxToPop)
	{
		size_t result = mBufferRest.PopFirstN(maxToPop);
		if(result == maxToPop) return result;
		
		maxToPop -= result;
		if(mOffset+maxToPop > mSize) maxToPop = size_t(mSize-mOffset);
		mOffset += maxToPop;
		result += maxToPop;
		loadBuffer();
		return result;
	}

	size_t CopyAdvanceToAdvance(Span<char>& dst)
	{
		size_t totalBytesRead = Algo::CopyAdvanceToAdvance(mBufferRest, dst);
		if(!mBufferRest.Empty()) return totalBytesRead;

		if(dst.Length() >= mBuffer.Length())
		{
			const size_t bytesRead = mFile->ReadData(mOffset, dst.Data(), dst.Length());
			mOffset += bytesRead;
			dst.Begin += bytesRead;
			totalBytesRead += bytesRead;
		}
		loadBuffer();
		totalBytesRead += Algo::CopyAdvanceToAdvance(mBufferRest, dst);
		return totalBytesRead;
	}

	template<typename AR> Meta::EnableIf<
		Range::IsArrayRangeOfExactly<AR, char>::_ && !Meta::IsConst<AR>::_,
	size_t> CopyAdvanceToAdvance(AR& dst)
	{
		Span<char> dstArr = {dst.Data(), dst.Length()};
		size_t result = CopyAdvanceToAdvance(dstArr);
		Range::PopFirstExactly(dst, result);
		return result;
	}

	forceinline ulong64 PositionInFile() const {return mOffset-mBufferRest.Length();}
	forceinline CSpan<char> BufferedData() const {return mBufferRest;}

	forceinline const Shared<OsFile>& File() const {return mFile;}

private:
	void loadBuffer()
	{
		const size_t bytesRead = mFile->ReadData(mOffset, mBuffer.Data(), mBuffer.Length());
		mOffset += bytesRead;
		mBufferRest = mBuffer(0, bytesRead);
	}

	Shared<OsFile> mFile;
	ulong64 mOffset, mSize;
	Array<char> mBuffer;
	Span<char> mBufferRest;
};

}}

INTRA_WARNING_POP
