#pragma once

#include "Extra/Utils/Shared.h"

#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Stream/InputStreamMixin.h"
#include "Intra/Range/Stream/Parse.h"

#include "Extra/Container/Sequential/Array.h"

#include "OsFile.h"

INTRA_BEGIN
class OsFile;
class FileReader: public InputStreamMixin<FileReader, char>
{
public:
	INTRA_FORCEINLINE FileReader(decltype(null)=null): mOffset(0), mSize(0) {}

	INTRA_FORCEINLINE FileReader(Shared<OsFile> file, Index bufferSize = 4096):
		mFile(Move(file)), mOffset(0), mSize(mFile == null? 0: mFile->Size())
	{
		mBuffer.SetCountUninitialized(bufferSize);
		loadBuffer(IgnoreErrors);
	}

	INTRA_FORCEINLINE FileReader(const FileReader& rhs) {operator=(rhs);}
	INTRA_FORCEINLINE FileReader(FileReader&& rhs) {operator=(Move(rhs));}

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

	FileReader& operator=(decltype(null))
	{
		mFile = null;
		mOffset = 0;
		mSize = 0;
		mBuffer = null;
		mBufferRest = null;
		return *this;
	}

	INTRA_FORCEINLINE char First() const {return mBufferRest.First();}
	
	INTRA_FORCEINLINE bool Empty() const noexcept {return mBufferRest.Empty();}

	INTRA_FORCEINLINE index_t Length() const {return index_t(mSize - PositionInFile());}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const noexcept {return Empty();}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const noexcept {return !Empty();}
	INTRA_FORCEINLINE explicit operator bool() const noexcept {return !Empty();}
	
	void PopFirst(ErrorReporter err = IgnoreErrors)
	{
		INTRA_DEBUG_ASSERT(!Empty());
		mBufferRest.PopFirst();
		if(!mBufferRest.Empty()) return;
		loadBuffer(err);
	}

	index_t PopFirstCount(ClampedSize maxToPop, ErrorReporter err = IgnoreErrors)
	{
		auto elementsToPop = FMin(size_t(maxToPop), size_t(Length()));
		auto result = size_t(mBufferRest.PopFirstCount(elementsToPop));
		if(result == elementsToPop) return index_t(result);
		
		elementsToPop -= result;
		if(mOffset + elementsToPop > mSize) elementsToPop = size_t(mSize - mOffset);
		mOffset += elementsToPop;
		result += elementsToPop;
		loadBuffer(err);
		return index_t(result);
	}

	index_t ReadWrite(Span<char>& dst, ErrorReporter err = IgnoreErrors)
	{
		index_t totalBytesRead = Intra::ReadWrite(mBufferRest, dst);
		if(!mBufferRest.Empty()) return totalBytesRead;

		if(dst.Length() >= mBuffer.Length())
		{
			const auto bytesRead = mFile->ReadData(mOffset, dst.Data(), size_t(dst.Length()), err);
			mOffset += bytesRead;
			dst.Begin += bytesRead;
			totalBytesRead += index_t(bytesRead);
		}
		loadBuffer(err);
		totalBytesRead += Intra::ReadWrite(mBufferRest, dst);
		return totalBytesRead;
	}

	template<typename AR> Requires<
		CArrayRangeOfExactly<AR, char> &&
		!CConst<AR>,
	index_t> ReadWrite(AR& dst, ErrorReporter err = IgnoreErrors)
	{
		Span<char> dstArr = SpanOf(dst);
		const auto result = ReadWrite(dstArr, err);
		PopFirstExactly(dst, result);
		return result;
	}

	uint64 PositionInFile() const {return mOffset - uint64(mBufferRest.Length());}
	CSpan<char> BufferedData() const {return mBufferRest;}

	const Shared<OsFile>& File() const {return mFile;}

private:
	void loadBuffer(ErrorReporter err)
	{
		const auto bytesRead = mFile->ReadData(mOffset, mBuffer.Data(), size_t(mBuffer.Length()), err);
		mOffset += bytesRead;
		mBufferRest = mBuffer.Take(index_t(bytesRead));
	}

	Shared<OsFile> mFile;
	uint64 mOffset, mSize;
	Array<char> mBuffer;
	Span<char> mBufferRest;
};
INTRA_END
