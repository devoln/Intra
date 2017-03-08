#pragma once

#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Generators/StringView.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

class BasicFileMapping
{
public:
	size_t Length() const {return mSize;}

	void Close();

	bool operator==(null_t) const {return mData==null;}
	bool operator!=(null_t) const {return mData!=null;}

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
	BasicFileMapping(StringView fileName, ulong64 startByte, size_t bytes, bool writeAccess);
	~BasicFileMapping() {Close();}
};

class FileMapping: public BasicFileMapping
{
public:
	enum: size_t {MAP_ALL};

	FileMapping(StringView fileName, ulong64 startByte, size_t bytes):
		BasicFileMapping(fileName, startByte, bytes, false) {}

	FileMapping(StringView fileName):
		BasicFileMapping(fileName, 0, ~size_t(0), false) {}

	FileMapping(FileMapping&& rhs) {assign(Meta::Move(rhs));}
	FileMapping(const FileMapping&) = delete;

	FileMapping& operator=(FileMapping&& rhs) {assign(Meta::Move(rhs)); return *this;}
	FileMapping& operator=(const FileMapping&) = delete;

	const byte* Data() const {return static_cast<byte*>(mData);}

	ArrayRange<const byte> AsRange() const {return {Data(), Length()};}

	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	ArrayRange<const T>> AsRangeOf() const {return {static_cast<T*>(mData), mSize/sizeof(T)};}
};

class WritableFileMapping: public BasicFileMapping
{
public:
	WritableFileMapping(StringView fileName, ulong64 startByte, size_t bytes):
		BasicFileMapping(fileName, startByte, bytes, true) {}

	WritableFileMapping(StringView fileName):
		BasicFileMapping(fileName, 0, ~size_t(0), true) {}

	WritableFileMapping(WritableFileMapping&& rhs) {assign(Meta::Move(rhs));}
	WritableFileMapping(const WritableFileMapping&) = delete;

	WritableFileMapping& operator=(WritableFileMapping&& rhs) {assign(Meta::Move(rhs)); return *this;}
	WritableFileMapping& operator=(const WritableFileMapping&) = delete;

	void Flush();

	byte* Data() const {return static_cast<byte*>(mData);}

	ArrayRange<byte> AsRange() const {return{Data(), Length()};}

	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	ArrayRange<T>> AsRangeOf() const {return {static_cast<T*>(mData), mSize/sizeof(T)};}
};

}}

INTRA_WARNING_POP
