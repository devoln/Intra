#pragma once

#include "OsFile.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Container/Sequential/Array.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Stream/Operators.h"
#include "Range/Stream/OutputStreamMixin.h"


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

class OsFile;

class FileWriter: public Range::OutputStreamMixin<FileWriter, char>
{
public:
	forceinline FileWriter(const OsFile& file, ulong64 startOffset, size_t bufferSize):
		mFile(&file), mOffset(startOffset)
	{
		mBuffer.SetCountUninitialized(bufferSize);
		mBufferRest = mBuffer;
	}

	forceinline FileWriter(const OsFile& file, ulong64 startOffset=0): FileWriter(file, startOffset, 4096) {}

	FileWriter(const FileWriter& rhs) = delete;
	FileWriter(FileWriter&& rhs) {operator=(Meta::Move(rhs));}

	FileWriter& operator=(const FileWriter& rhs) = delete;

	FileWriter& operator=(FileWriter&& rhs)
	{
		mFile = rhs.mFile;
		mOffset = rhs.mOffset;
		mBuffer = Meta::Move(rhs.mBuffer);
		mBufferRest = rhs.mBufferRest;
		rhs.mBufferRest = null;
		return *this;
	}

	~FileWriter() {Flush();}

	forceinline bool Empty() const {return mBufferRest.Empty();}
	
	forceinline void Put(char c)
	{
		INTRA_DEBUG_ASSERT(!Empty());
		mBufferRest.Put(c);
		if(mBufferRest.Empty()) Flush();
	}

	size_t CopyAdvanceFromAdvance(ArrayRange<const char>& src)
	{
		size_t totalBytesWritten = Algo::CopyAdvanceToAdvance(src, mBufferRest);
		if(!mBufferRest.Empty()) return totalBytesWritten;

		Flush();
		if(src.Length() >= mBuffer.Length())
		{
			const size_t bytesWritten = mFile->WriteData(mOffset, src.Data(), src.Length());
			mOffset += bytesWritten;
			src.Begin += bytesWritten;
			totalBytesWritten += bytesWritten;
		}
		totalBytesWritten += Algo::CopyAdvanceToAdvance(src, mBufferRest);
		return totalBytesWritten;
	}

	template<typename AR> Meta::EnableIf<
		Range::IsArrayRangeOfExactly<AR, char>::_ && !Meta::IsConst<AR>::_,
	size_t> CopyAdvanceFromAdvance(AR& src)
	{
		ArrayRange<const char> srcArr = {src.Data(), src.Length()};
		size_t result = CopyAdvanceFromAdvance(srcArr);
		Range::PopFirstExactly(src, result);
		return result;
	}

	void Flush()
	{
		const size_t bytesWritten = mBuffer.Length()-mBufferRest.Length();
		mFile->SetSize(mOffset+bytesWritten);
		mFile->WriteData(mOffset, mBuffer.Data(), bytesWritten);
		mOffset += bytesWritten;
		mBufferRest = mBuffer;
	}

	forceinline ulong64 PositionInFile() const {return mOffset+mBuffer.Length()-mBufferRest.Length();}
	forceinline ulong64 FlushedPositionInFile() const {return mOffset;}

	forceinline const OsFile& File() const {return *mFile;}

private:
	const OsFile* mFile;
	ulong64 mOffset;
	Array<char> mBuffer;
	ArrayRange<char> mBufferRest;
};

}}

INTRA_WARNING_POP
