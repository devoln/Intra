#pragma once

#include "Meta/Type.h"
#include "Range/Generators/ArrayRange.h"

namespace Intra { namespace IO {

class BasicFileMapping
{
public:
	size_t Length() const {return mSize;}

	void Close();

protected:
	void* mData;
	size_t mSize;

	void assign(BasicFileMapping&& rhs)
	{
		mData = rhs.mData;
		mSize = rhs.mSize;
		rhs.mData = null;
	}

	BasicFileMapping() {}
	BasicFileMapping(StringView fileName, size_t startByte, size_t bytes, bool writeAccess);
	~BasicFileMapping() {Close();}
};

class FileMapping: public BasicFileMapping
{
public:
	FileMapping(StringView fileName, size_t startByte, size_t bytes):
		BasicFileMapping(fileName, startByte, bytes, false) {}

	FileMapping(FileMapping&& rhs) {assign(Meta::Move(rhs));}
	FileMapping(const FileMapping&) = delete;

	FileMapping& operator=(FileMapping&& rhs) {assign(Meta::Move(rhs)); return *this;}
	FileMapping& operator=(const FileMapping&) = delete;

	const byte* Data() const {return static_cast<byte*>(mData);}

	ArrayRange<const byte> AsRange() const {return{Data(), Length()};}

	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	ArrayRange<const T>> AsRangeOf() const {return {static_cast<T*>(mData), mSize/sizeof(T)};}
};

class WritableFileMapping: public BasicFileMapping
{
public:
	WritableFileMapping(StringView fileName, size_t startByte, size_t bytes):
		BasicFileMapping(fileName, startByte, bytes, true) {}

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
