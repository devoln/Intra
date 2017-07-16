#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Utils/Shared.h"

#include "Range/Mutation/Copy.h"
#include "Range/Stream/InputStreamMixin.h"
#include "Range/Stream/Parse.h"

#include "Container/Sequential/Array.h"

#include "OsFile.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

using Range::operator>>;

class OsFile;

class FileReader: public Range::InputStreamMixin<FileReader, char>
{
public:
	forceinline FileReader(null_t=null): mOffset(0), mSize(0) {}

	forceinline FileReader(Shared<OsFile> file, size_t bufferSize = 4096):
		mFile(Cpp::Move(file)), mOffset(0), mSize(mFile == null? 0: mFile->Size())
	{
		mBuffer.SetCountUninitialized(bufferSize);
		loadBuffer(Error::Skip());
	}

	forceinline FileReader(const FileReader& rhs) {operator=(rhs);}
	forceinline FileReader(FileReader&& rhs) {operator=(Cpp::Move(rhs));}

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
		mFile = Cpp::Move(rhs.mFile);
		mOffset = rhs.mOffset;
		mSize = rhs.mSize;
		mBuffer = Cpp::Move(rhs.mBuffer);
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

	forceinline size_t Length() const {return size_t(mSize - PositionInFile());}

	forceinline bool operator==(null_t) const noexcept {return Empty();}
	forceinline bool operator!=(null_t) const noexcept {return !Empty();}
	forceinline explicit operator bool() const noexcept {return !Empty();}
	
	void PopFirst(ErrorStatus& status = Error::Skip())
	{
		INTRA_DEBUG_ASSERT(!Empty());
		mBufferRest.PopFirst();
		if(!mBufferRest.Empty()) return;
		loadBuffer(status);
	}

	size_t PopFirstN(size_t maxToPop, ErrorStatus& status = Error::Skip())
	{
		size_t result = mBufferRest.PopFirstN(maxToPop);
		if(result == maxToPop) return result;
		
		maxToPop -= result;
		if(mOffset+maxToPop > mSize) maxToPop = size_t(mSize-mOffset);
		mOffset += maxToPop;
		result += maxToPop;
		loadBuffer(status);
		return result;
	}

	size_t ReadWrite(Span<char>& dst, ErrorStatus& status = Error::Skip())
	{
		size_t totalBytesRead = Range::ReadWrite(mBufferRest, dst);
		if(!mBufferRest.Empty()) return totalBytesRead;

		if(dst.Length() >= mBuffer.Length())
		{
			const size_t bytesRead = mFile->ReadData(mOffset, dst.Data(), dst.Length(), status);
			mOffset += bytesRead;
			dst.Begin += bytesRead;
			totalBytesRead += bytesRead;
		}
		loadBuffer(status);
		totalBytesRead += Range::ReadWrite(mBufferRest, dst);
		return totalBytesRead;
	}

	template<typename AR> Meta::EnableIf<
		Concepts::IsArrayRangeOfExactly<AR, char>::_ &&
		!Meta::IsConst<AR>::_,
	size_t> ReadWrite(AR& dst, ErrorStatus& status = Error::Skip())
	{
		Span<char> dstArr = {dst.Data(), dst.Length()};
		const size_t result = ReadWrite(dstArr, status);
		Range::PopFirstExactly(dst, result);
		return result;
	}

	forceinline ulong64 PositionInFile() const {return mOffset - mBufferRest.Length();}
	forceinline CSpan<char> BufferedData() const {return mBufferRest;}

	forceinline const Shared<OsFile>& File() const {return mFile;}

private:
	void loadBuffer(ErrorStatus& status)
	{
		const size_t bytesRead = mFile->ReadData(mOffset, mBuffer.Data(), mBuffer.Length(), status);
		mOffset += bytesRead;
		mBufferRest = mBuffer.Take(bytesRead);
	}

	Shared<OsFile> mFile;
	ulong64 mOffset, mSize;
	Array<char> mBuffer;
	Span<char> mBufferRest;
};

}}

INTRA_WARNING_POP
