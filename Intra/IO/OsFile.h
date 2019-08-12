#pragma once



#include "Core/Range/StringView.h"
#include "Core/Range/Span.h"
#include "Core/Assert.h"
#include "System/Error.h"

#include "Core/Range/Search/Single.h"
#include "Core/Range/Stream/RawRead.h"
#include "Core/Range/Mutation/Fill.h"

#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace IO {

class OsFile
{
public:
	enum class Mode {Read, Write, ReadWrite, None};

	OsFile(StringView fileName, Mode mode, bool disableSystemBuffering, ErrorStatus& status);
	OsFile(StringView fileName, Mode mode, ErrorStatus& status): OsFile(fileName, mode, false, status) {}
	OsFile(null_t=null): mHandle(null), mMode(Mode::None), mOwning(false) {}
	~OsFile() {Close();}
	
	OsFile(OsFile&& rhs):
		mHandle(rhs.mHandle), mMode(rhs.mMode), mOwning(rhs.mOwning)
	{rhs.mHandle = null; rhs.mOwning = false;}
	
	OsFile(const OsFile&) = delete;
	
	OsFile& operator=(OsFile&& rhs)
	{
		Close();
		mHandle = rhs.mHandle;
		rhs.mHandle = null;
		mMode = rhs.mMode;
		return *this;
	}
	OsFile& operator=(const OsFile&) = delete;

	bool operator==(null_t) const {return mHandle==null;}
	bool operator!=(null_t) const {return mHandle!=null;}

	forceinline OsFile& operator=(null_t) {Close(); return *this;}

	//! ������� ����, ��������������� � ���� ��������.
	//! ������� ������ ��������� � null ���������.
	void Close();

	//! ��������� ������ �� ����� �� ���������� ��������.
	//! @param fileOffset �������� ������ ����������� ������ � �����.
	//! @param data ��������� �� ������ ������������ �� ����� ����� ������.
	//! @param bytes ������ ����������� ������ � ������.
	//! @return ���������� ����������� ����.
	size_t ReadData(uint64 fileOffset, void* data, size_t bytes, ErrorStatus& status) const;
	
	//! ���������� ������ ����� ��� 0, ���� ��� �� ����.
	uint64 Size(ErrorStatus& status = Error::Skip()) const;

	//! �������� ������ � ���� �� ���������� ��������.
	//! @param fileOffset �������� ������ ������������ ������ � �����.
	//! @param data ��������� �� ������ ������������� � ���� ����� ������.
	//! @param bytes ������ ������������ ������ � ������.
	//! @return ���������� ���������� ����.
	size_t WriteData(uint64 fileOffset, const void* data, size_t bytes, ErrorStatus& status) const;

	//! ���������� ������ ����� ������ size.
	void SetSize(uint64 size, ErrorStatus& status) const;

	//! ��������� ���� ������� � ������.
	//! @param fileName ���� � �����.
	//! @param[out] oFileOpened ����������, ��� �� ������ ����.
	//! @return ������ � ���������� ����� ����� ��� ������ ������, ���� ���� �� ��� ������.
	static String ReadAsString(StringView fileName, ErrorStatus& status);

	struct NativeData;
	typedef NativeData* NativeHandle;

	//! ���������� ����� ����� ��.
	//! ��� Windows ������������ �������� ����� ��������� � HANDLE (�������� ����� CreateFile).
	//! ��� ��������� �� ������������ �������� ����� ��������� � int (file descriptor, �������� ����� open).
	//! �������� ������������� ������ ������� ������� � ��� ������ ���������, ���� � ���� ���� ��������.
	NativeHandle GetNativeHandle() const {return mHandle;}

	//! ������ ������ OsFile �� ����������� ������ ��
	//! @param handle ����� �� ���� HANDLE (��� WinAPI) ��� int (file descriptor, ��� �� ����� Windows).
	//! @param owning ���� true, ���������� handle ����� ������ ��������� �������� �������������.
	static OsFile FromNative(NativeHandle handle, bool owning);

	bool OwnsHandle() const
	{
		INTRA_DEBUG_ASSERT(!mOwning || mHandle != null);
		return mOwning;
	}

	StringView FullPath() const {return mFullPath;}

private:
	NativeHandle mHandle;
	Mode mMode;
	bool mOwning;
	String mFullPath;
};

}}

INTRA_WARNING_POP
