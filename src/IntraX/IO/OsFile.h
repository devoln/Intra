#pragma once

#include "Intra/Range/StringView.h"
#include "Intra/Range/Span.h"
#include "Intra/Assert.h"
#include "IntraX/System/Error.h"

#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Stream/RawRead.h"
#include "Intra/Range/Mutation/Fill.h"

#include "IntraX/Container/Sequential/String.h"

namespace Intra { INTRA_BEGIN
class OsFile
{
public:
	enum class Mode {Read, Write, ReadWrite, None};

	OsFile(StringView fileName, Mode mode, bool disableSystemBuffering, ErrorReporter err);
	OsFile(StringView fileName, Mode mode, ErrorReporter err): OsFile(fileName, mode, false, err) {}
	OsFile() = default;
	~OsFile() {Close();}
	
	OsFile(OsFile&& rhs): mHandle(rhs.mHandle), mMode(rhs.mMode), mOwning(rhs.mOwning)
	{rhs.mHandle = nullptr; rhs.mOwning = false;}
	
	OsFile(const OsFile&) = delete;
	
	OsFile& operator=(OsFile&& rhs)
	{
		Close();
		mHandle = rhs.mHandle;
		rhs.mHandle = nullptr;
		mMode = rhs.mMode;
		return *this;
	}
	OsFile& operator=(const OsFile&) = delete;

	explicit operator bool() const {return mHandle != nullptr;}

	void Close();

	size_t ReadData(uint64 fileOffset, void* data, size_t bytes, ErrorReporter err) const;
	
	uint64 Size(ErrorReporter err = IgnoreErrors) const;

	size_t WriteData(uint64 fileOffset, const void* data, size_t bytes, ErrorReporter err) const;

	void SetSize(uint64 size, ErrorReporter err) const;

	static String ReadAsString(StringView fileName, ErrorReporter err);

	struct NativeData;
	using NativeHandle = NativeData*;

	NativeHandle GetNativeHandle() const {return mHandle;}

	static OsFile FromNative(NativeHandle handle, bool owning);

	bool OwnsHandle() const
	{
		INTRA_PRECONDITION(!mOwning || *this);
		return mOwning;
	}

	StringView FullPath() const {return mFullPath;}

private:
	NativeHandle mHandle = nullptr;
	Mode mMode = Mode::None;
	bool mOwning = false;
	String mFullPath;
};
} INTRA_END
