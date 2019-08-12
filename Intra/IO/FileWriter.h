#pragma once

#include "OsFile.h"

#include "Core/Core.h"


#include "Container/Sequential/Array.h"

#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Stream/ToString.h"
#include "Core/Range/Stream/OutputStreamMixin.h"

#include "Utils/Shared.h"
#include "Core/Range/Span.h"


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace IO {

using Range::operator<<;

//! Буферизованный output-поток для записи в файл.
class FileWriter: public Range::OutputStreamMixin<FileWriter, char>
{
public:
	forceinline FileWriter(null_t=null): mOffset(0) {}

	forceinline FileWriter(Shared<OsFile> file, uint64 startOffset=0, size_t bufferSize=4096):
		mFile(Move(file)), mOffset(startOffset)
	{
		if(mFile == null) return;
		mBuffer.SetCountUninitialized(bufferSize);
		mBufferRest = mBuffer;
	}

	forceinline static FileWriter Append(Shared<OsFile> file, size_t bufferSize=4096)
	{
		if(file == null) return null;
		const uint64 size = file->Size();
		return FileWriter(Move(file), size, bufferSize);
	}

	forceinline static FileWriter Overwrite(Shared<OsFile> file, size_t bufferSize=4096)
	{
		if(file == null) return null;
		file->SetSize(0, Error::Skip());
		return FileWriter(Move(file), 0, bufferSize);
	}

	FileWriter(const FileWriter&) = delete;
	forceinline FileWriter(FileWriter&& rhs) {operator=(Move(rhs));}

	forceinline bool operator==(null_t) {return mFile == null;}
	forceinline bool operator!=(null_t) {return mFile != null;}

	FileWriter& operator=(const FileWriter& rhs) = delete;

	FileWriter& operator=(FileWriter&& rhs)
	{
		Flush(Error::Skip());
		mFile = Move(rhs.mFile);
		mOffset = rhs.mOffset;
		mBuffer = Move(rhs.mBuffer);
		mBufferRest = rhs.mBufferRest;
		rhs.mBufferRest = null;
		return *this;
	}

	FileWriter& operator=(null_t)
	{
		Flush(Error::Skip());
		mFile = null;
		mBuffer = null;
		mBufferRest = null;
		mOffset = 0;
		return *this;
	}

	forceinline ~FileWriter() {Flush(Error::Skip());}

	forceinline bool Full() const {return mBufferRest.Empty();}
	
	forceinline void Put(char c, ErrorStatus& status = Error::Skip())
	{
		INTRA_DEBUG_ASSERT(!Full());
		mBufferRest.Put(c);
		if(mBufferRest.Empty()) Flush(status);
	}

	size_t PutAllAdvance(CSpan<char>& src, ErrorStatus& status = Error::Skip())
	{
		size_t totalBytesWritten = Range::ReadWrite(src, mBufferRest);
		if(!mBufferRest.Empty()) return totalBytesWritten;

		Flush(status);
		if(src.Length() >= mBuffer.Length())
		{
			const size_t bytesWritten = mFile->WriteData(mOffset, src.Data(), src.Length(), status);
			mOffset += bytesWritten;
			src.Begin += bytesWritten;
			totalBytesWritten += bytesWritten;
		}
		totalBytesWritten += Range::ReadWrite(src, mBufferRest);
		return totalBytesWritten;
	}

	template<typename AR> Requires<
		CArrayRangeOfExactly<AR, char>::_ &&
		!CConst<AR>::_,
	size_t> PutAllAdvance(AR& src, ErrorStatus& status = Error::Skip())
	{
		CSpan<char> srcArr = {src.Data(), src.Length()};
		size_t result = PutAllAdvance(srcArr, status);
		Range::PopFirstExactly(src, result);
		return result;
	}

	//! Записать буфер в файл.
	void Flush(ErrorStatus& status = Error::Skip())
	{
		if(mBuffer.Empty()) return;
		const size_t bytesWritten = mBuffer.Length() - mBufferRest.Length();
		mFile->SetSize(mOffset + bytesWritten, status);
		mFile->WriteData(mOffset, mBuffer.Data(), bytesWritten, status);
		mOffset += bytesWritten;
		mBufferRest = mBuffer;
	}

	forceinline uint64 PositionInFile() const {return mOffset + mBuffer.Length() - mBufferRest.Length();}
	forceinline uint64 FlushedPositionInFile() const {return mOffset;}

	forceinline const Shared<OsFile>& File() const {return mFile;}

private:
	Shared<OsFile> mFile;
	uint64 mOffset;
	Array<char> mBuffer;
	Span<char> mBufferRest;
};

}}

INTRA_WARNING_POP
