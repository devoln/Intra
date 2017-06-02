#pragma once

#include "Cpp/Warnings.h"

#include "Meta/Type.h"

#include "Utils/Span.h"
#include "Utils/StringView.h"
#include "Utils/ErrorStatus.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

class BasicFileMapping
{
public:
	size_t Length() const {return mSize;}

	void Close();

	bool operator==(null_t) const {return mData == null;}
	bool operator!=(null_t) const {return mData != null;}

protected:
	void* mData;
	size_t mSize;

	void assign(BasicFileMapping&& rhs)
	{
		mData = rhs.mData;
		mSize = rhs.mSize;
		rhs.mData = null;
		rhs.mSize = 0;
	}

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

	FileMapping(FileMapping&& rhs) {assign(Cpp::Move(rhs));}
	FileMapping(const FileMapping&) = delete;

	FileMapping& operator=(FileMapping&& rhs) {assign(Cpp::Move(rhs)); return *this;}
	FileMapping& operator=(const FileMapping&) = delete;

	const byte* Data() const {return static_cast<byte*>(mData);}

	CSpan<byte> AsRange() const {return {Data(), Length()};}

	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	CSpan<T>> AsRangeOf() const {return {static_cast<T*>(mData), mSize/sizeof(T)};}
};

class WritableFileMapping: public BasicFileMapping
{
public:
	WritableFileMapping(StringView fileName, ulong64 startByte, size_t bytes, ErrorStatus& status):
		BasicFileMapping(fileName, startByte, bytes, true, status) {}

	WritableFileMapping(StringView fileName, ErrorStatus& status):
		BasicFileMapping(fileName, 0, ~size_t(0), true, status) {}

	WritableFileMapping(WritableFileMapping&& rhs) {assign(Cpp::Move(rhs));}
	WritableFileMapping(const WritableFileMapping&) = delete;

	WritableFileMapping& operator=(WritableFileMapping&& rhs) {assign(Cpp::Move(rhs)); return *this;}
	WritableFileMapping& operator=(const WritableFileMapping&) = delete;

	void Flush();

	byte* Data() const {return static_cast<byte*>(mData);}

	Span<byte> AsRange() const {return{Data(), Length()};}

	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	Span<T>> AsRangeOf() const {return {static_cast<T*>(mData), mSize/sizeof(T)};}
};

}}

INTRA_WARNING_POP
