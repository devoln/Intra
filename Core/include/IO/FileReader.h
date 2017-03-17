#pragma once

#include "OsFile.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Container/Sequential/Array.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Stream/Operators.h"
#include "Range/Stream/InputStreamMixin.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {


class OsFile;

class FileReader: Range::InputStreamMixin<FileReader, char>
{
public:
	forceinline FileReader(const OsFile& file, size_t bufferSize):
		mFile(&file), mOffset(0), mSize(file.Size())
	{
		mBuffer.SetCountUninitialized(bufferSize);
		loadBuffer();
	}

	forceinline FileReader(const OsFile& file): FileReader(file, 4096) {}

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
		mFile = rhs.mFile;
		mOffset = rhs.mOffset;
		mSize = rhs.mSize;
		mBuffer = Meta::Move(rhs.mBuffer);
		mBufferRest = rhs.mBufferRest;
		rhs.mBufferRest = null;
		return *this;
	}

	forceinline char First() const {return mBufferRest.First();}
	
	forceinline bool Empty() const {return mBufferRest.Empty();}

	forceinline size_t Length() const {return size_t(mSize+PositionInFile());}
	
	void PopFirst()
	{
		INTRA_ASSERT(!Empty());
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

	size_t CopyAdvanceToAdvance(ArrayRange<char>& dst)
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
		ArrayRange<char> dstArr = {dst.Data(), dst.Length()};
		size_t result = CopyAdvanceToAdvance(dstArr);
		Range::PopFirstExactly(dst, result);
		return result;
	}

	forceinline ulong64 PositionInFile() const {return mOffset-mBufferRest.Length();}
	forceinline ArrayRange<const char> BufferedData() const {return mBufferRest;}

	forceinline const OsFile& File() const {return *mFile;}

private:
	void loadBuffer()
	{
		const size_t bytesRead = mFile->ReadData(mOffset, mBuffer.Data(), mBuffer.Length());
		mOffset += bytesRead;
		mBufferRest = mBuffer(0, bytesRead);
	}

	const OsFile* mFile;
	ulong64 mOffset, mSize;
	Array<char> mBuffer;
	ArrayRange<char> mBufferRest;
};

}}

INTRA_WARNING_POP
