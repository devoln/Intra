#pragma once

#include "OsFile.h"

#include "IntraX/Container/Sequential/Array.h"

#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Stream/ToString.h"
#include "Intra/Range/Stream/OutputStreamMixin.h"

#include "IntraX/Utils/Shared.h"
#include "Intra/Range/Span.h"

INTRA_BEGIN
class FileWriter: public OutputStreamMixin<FileWriter, char>
{
public:
	FileWriter() = default;
	FileWriter(decltype(null)) {}

	FileWriter(Shared<OsFile> file, uint64 startOffset = 0, Index bufferSize = 4096):
		mFile(Move(file)), mOffset(startOffset)
	{
		if(mFile == null) return;
		mBuffer.SetCountUninitialized(bufferSize);
		mBufferRest = mBuffer;
	}

	static FileWriter Append(Shared<OsFile> file, Index bufferSize = 4096)
	{
		if(file == null) return null;
		const uint64 size = file->Size();
		return FileWriter(Move(file), size, bufferSize);
	}

	static FileWriter Overwrite(Shared<OsFile> file, Index bufferSize = 4096)
	{
		if(file == null) return null;
		file->SetSize(0, IgnoreErrors);
		return FileWriter(Move(file), 0, bufferSize);
	}

	FileWriter(const FileWriter&) = delete;
	FileWriter(FileWriter&& rhs) {operator=(Move(rhs));}

	bool operator==(decltype(null)) {return mFile == null;}
	bool operator!=(decltype(null)) {return mFile != null;}

	FileWriter& operator=(const FileWriter& rhs) = delete;

	FileWriter& operator=(FileWriter&& rhs)
	{
		Flush(IgnoreErrors);
		mFile = Move(rhs.mFile);
		mOffset = rhs.mOffset;
		mBuffer = Move(rhs.mBuffer);
		mBufferRest = rhs.mBufferRest;
		rhs.mBufferRest = null;
		return *this;
	}

	FileWriter& operator=(decltype(null))
	{
		Flush(IgnoreErrors);
		mFile = null;
		mBuffer = null;
		mBufferRest = null;
		mOffset = 0;
		return *this;
	}

	INTRA_FORCEINLINE ~FileWriter() {Flush(IgnoreErrors);}

	INTRA_FORCEINLINE bool Full() const {return mBufferRest.Empty();}
	
	INTRA_FORCEINLINE void Put(char c, ErrorReporter err = IgnoreErrors)
	{
		INTRA_PRECONDITION(!Full());
		mBufferRest.Put(c);
		if(mBufferRest.Empty()) Flush(err);
	}

	index_t PutAllAdvance(CSpan<char>& src, ErrorReporter err = IgnoreErrors)
	{
		auto totalBytesWritten = ReadWrite(src, mBufferRest);
		if(!mBufferRest.Empty()) return totalBytesWritten;

		Flush(err);
		if(src.Length() >= mBuffer.Length())
		{
			const size_t bytesWritten = mFile->WriteData(mOffset, src.Data(), size_t(src.Length()), err);
			mOffset += bytesWritten;
			src.Begin += bytesWritten;
			totalBytesWritten += bytesWritten;
		}
		totalBytesWritten += ReadWrite(src, mBufferRest);
		return totalBytesWritten;
	}

	template<typename AR> requires
		CSame<TArrayElement<AR>, char> &&
		(!CConst<AR>)
	size_t PutAllAdvance(AR& src, ErrorReporter err = IgnoreErrors)
	{
		CSpan<char> srcArr = {src.Data(), src.Length()};
		size_t result = PutAllAdvance(srcArr, err);
		PopFirstExactly(src, result);
		return result;
	}

	/// �������� ����� � ����.
	void Flush(ErrorReporter err = IgnoreErrors)
	{
		if(mBuffer.Empty()) return;
		const auto bytesWritten = mBuffer.Length() - mBufferRest.Length();
		mFile->SetSize(mOffset + size_t(bytesWritten), err);
		mFile->WriteData(mOffset, mBuffer.Data(), size_t(bytesWritten), err);
		mOffset += size_t(bytesWritten);
		mBufferRest = mBuffer;
	}

	uint64 PositionInFile() const {return mOffset + size_t(mBuffer.Length() - mBufferRest.Length());}
	uint64 FlushedPositionInFile() const {return mOffset;}

	const Shared<OsFile>& File() const {return mFile;}

private:
	Shared<OsFile> mFile;
	uint64 mOffset = 0;
	Array<char> mBuffer;
	Span<char> mBufferRest;
};
INTRA_END
