#pragma once

#include "Utils/Shared.h"

#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Stream/InputStreamMixin.h"
#include "Core/Range/Stream/Parse.h"

#include "Container/Sequential/Array.h"

#include "OsFile.h"

INTRA_BEGIN
class OsFile;
class FileReader: public InputStreamMixin<FileReader, char>
{
public:
	forceinline FileReader(null_t=null): mOffset(0), mSize(0) {}

	forceinline FileReader(Shared<OsFile> file, size_t bufferSize = 4096):
		mFile(Move(file)), mOffset(0), mSize(mFile == null? 0: mFile->Size())
	{
		mBuffer.SetCountUninitialized(bufferSize);
		loadBuffer(IgnoreErrors);
	}

	forceinline FileReader(const FileReader& rhs) {operator=(rhs);}
	forceinline FileReader(FileReader&& rhs) {operator=(Move(rhs));}

	FileReader& operator=(const FileReader& rhs)
	{
		mFile = rhs.mFile;
		mOffset = rhs.mOffset;
		mSize = rhs.mSize;
		mBuffer.SetCountUninitialized(rhs.mBuffer.Length());
		mBufferRest = mBuffer.Take(rhs.mBufferRest.Length());
		CopyTo(rhs.mBufferRest, mBufferRest);
		return *this;
	}

	FileReader& operator=(FileReader&& rhs)
	{
		mFile = Move(rhs.mFile);
		mOffset = rhs.mOffset;
		mSize = rhs.mSize;
		mBuffer = Move(rhs.mBuffer);
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
	
	forceinline bool Empty() const noexcept {return mBufferRest.Empty();}

	forceinline index_t Length() const {return size_t(mSize - PositionInFile());}

	forceinline bool operator==(null_t) const noexcept {return Empty();}
	forceinline bool operator!=(null_t) const noexcept {return !Empty();}
	forceinline explicit operator bool() const noexcept {return !Empty();}
	
	void PopFirst(ErrorReporter err = IgnoreErrors)
	{
		INTRA_DEBUG_ASSERT(!Empty());
		mBufferRest.PopFirst();
		if(!mBufferRest.Empty()) return;
		loadBuffer(err);
	}

	size_t PopFirstN(size_t maxToPop, ErrorReporter err = IgnoreErrors)
	{
		size_t result = mBufferRest.PopFirstN(maxToPop);
		if(result == maxToPop) return result;
		
		maxToPop -= result;
		if(mOffset+maxToPop > mSize) maxToPop = size_t(mSize-mOffset);
		mOffset += maxToPop;
		result += maxToPop;
		loadBuffer(err);
		return result;
	}

	size_t ReadWrite(Span<char>& dst, ErrorReporter err = IgnoreErrors)
	{
		size_t totalBytesRead = Intra::ReadWrite(mBufferRest, dst);
		if(!mBufferRest.Empty()) return totalBytesRead;

		if(dst.Length() >= mBuffer.Length())
		{
			const size_t bytesRead = mFile->ReadData(mOffset, dst.Data(), dst.Length(), err);
			mOffset += bytesRead;
			dst.Begin += bytesRead;
			totalBytesRead += bytesRead;
		}
		loadBuffer(err);
		totalBytesRead += Intra::ReadWrite(mBufferRest, dst);
		return totalBytesRead;
	}

	template<typename AR> Requires<
		CArrayRangeOfExactly<AR, char> &&
		!CConst<AR>,
	size_t> ReadWrite(AR& dst, ErrorReporter err = IgnoreErrors)
	{
		Span<char> dstArr = {dst.Data(), dst.Length()};
		const size_t result = ReadWrite(dstArr, err);
		PopFirstExactly(dst, result);
		return result;
	}

	forceinline uint64 PositionInFile() const {return mOffset - mBufferRest.Length();}
	forceinline CSpan<char> BufferedData() const {return mBufferRest;}

	forceinline const Shared<OsFile>& File() const {return mFile;}

private:
	void loadBuffer(ErrorReporter err)
	{
		const size_t bytesRead = mFile->ReadData(mOffset, mBuffer.Data(), mBuffer.Length(), err);
		mOffset += bytesRead;
		mBufferRest = mBuffer.Take(bytesRead);
	}

	Shared<OsFile> mFile;
	uint64 mOffset, mSize;
	Array<char> mBuffer;
	Span<char> mBufferRest;
};
INTRA_END
