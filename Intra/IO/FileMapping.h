#pragma once

#include "Cpp/Warnings.h"

#include "Meta/Type.h"

#include "Utils/Span.h"
#include "Utils/StringView.h"
#include "Utils/ErrorStatus.h"

#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

class BasicFileMapping
{
public:
	BasicFileMapping(BasicFileMapping&& rhs):
		mData(rhs.mData)
#ifdef INTRA_DEBUG
		, mFilePath(Cpp::Move(rhs.mFilePath))
#endif
	{rhs.mData = null;}

	BasicFileMapping& operator=(BasicFileMapping&& rhs)
	{
		Cpp::Swap(mData, rhs.mData);
#ifdef INTRA_DEBUG
		mFilePath = Cpp::Move(rhs.mFilePath);
#endif
		return *this;
	}

	BasicFileMapping(const BasicFileMapping&) = delete;
	BasicFileMapping& operator=(const BasicFileMapping&) = delete;

	size_t Length() const {return mData.Length();}

	void Close();

	bool operator==(null_t) const {return mData == null;}
	bool operator!=(null_t) const {return mData != null;}

protected:
	Span<byte> mData;
	String mFilePath;

	BasicFileMapping() {}
	BasicFileMapping(StringView fileName, ulong64 startByte, size_t bytes, bool writeAccess, ErrorStatus& status);
	~BasicFileMapping() {Close();}
};

class FileMapping: public BasicFileMapping
{
public:
	FileMapping(StringView fileName, ulong64 startByte, size_t bytes, ErrorStatus& status):
		BasicFileMapping(fileName, startByte, bytes, false, status) {}

	FileMapping(StringView fileName, ErrorStatus& status):
		BasicFileMapping(fileName, 0, ~size_t(0), false, status) {}

	FileMapping(FileMapping&&) = default;
	FileMapping(const FileMapping&) = delete;

	FileMapping& operator=(FileMapping&&) = default;
	FileMapping& operator=(const FileMapping&) = delete;

	const byte* Data() const {return mData.Data();}

	CSpan<byte> AsRange() const {return mData;}

	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	CSpan<T>> AsRangeOf() const {return CSpanOfRaw<T>(mData.Data(), mData.Length());}
};

class WritableFileMapping: public BasicFileMapping
{
public:
	WritableFileMapping(StringView fileName, ulong64 startByte, size_t bytes, ErrorStatus& status):
		BasicFileMapping(fileName, startByte, bytes, true, status) {}

	WritableFileMapping(StringView fileName, ErrorStatus& status):
		BasicFileMapping(fileName, 0, ~size_t(0), true, status) {}

	WritableFileMapping(WritableFileMapping&&) = default;
	WritableFileMapping(const WritableFileMapping&) = delete;

	WritableFileMapping& operator=(WritableFileMapping&&) = default;
	WritableFileMapping& operator=(const WritableFileMapping&) = delete;

	void Flush();

	byte* Data() const {return mData.Data();}

	Span<byte> AsRange() const {return mData;}

	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	Span<T>> AsRangeOf() const {return SpanOfRaw<T>(mData.Data(), mData.Length());}
};

}}

INTRA_WARNING_POP
