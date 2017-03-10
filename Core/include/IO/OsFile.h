#pragma once

#include "Algo/Search/Single.h"
#include "Algo/Raw/Read.h"
#include "Range/Generators/StringView.h"
#include "Range/Generators/ArrayRange.h"
#include "Algo/Mutation/Fill.h"

namespace Intra { namespace IO {

class FileReader;
class FileWriter;

class OsFile
{
public:
	enum class Mode {Read, Write, ReadWrite};

	OsFile(StringView fileName, Mode mode, bool disableSystemBuffering=false);
	~OsFile();
	
	OsFile(OsFile&& rhs): mHandle(rhs.mHandle) {rhs.mHandle = null;}
	OsFile(const OsFile&) = delete;
	
	OsFile& operator=(OsFile&& rhs)
	{
		mHandle = rhs.mHandle;
		rhs.mHandle = null;
		return *this;
	}
	OsFile& operator=(const OsFile&) = delete;

	bool operator==(null_t) const {return mHandle==null;}
	bool operator!=(null_t) const {return mHandle!=null;}

	size_t ReadData(ulong64 fileOffset, void* data, size_t bytes) const;
	ulong64 Size() const;
	size_t WriteData(ulong64 fileOffset, const void* data, size_t bytes) const;
	void SetSize(ulong64 size) const;

	FileReader Reader() const;
	FileWriter Writer() const;
	FileReader AsRange() const;

	static String ReadAsString(StringView fileName, bool& fileOpened);
	static String ReadAsString(StringView fileName) {bool unused; return ReadAsString(fileName, unused);}

private:
	struct Handle;
	Handle* mHandle;
	Mode mMode;
};

}}
