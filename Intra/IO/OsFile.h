#pragma once

#include "Cpp/Warnings.h"

#include "Utils/StringView.h"
#include "Utils/Span.h"
#include "Utils/Debug.h"
#include "Utils/ErrorStatus.h"

#include "Range/Search/Single.h"
#include "Range/Stream/RawRead.h"
#include "Range/Mutation/Fill.h"

#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

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
	size_t ReadData(ulong64 fileOffset, void* data, size_t bytes) const;
	
	//! ���������� ������ ����� ��� 0, ���� ��� �� ����.
	ulong64 Size() const;

	//! �������� ������ � ���� �� ���������� ��������.
	//! @param fileOffset �������� ������ ������������ ������ � �����.
	//! @param data ��������� �� ������ ������������� � ���� ����� ������.
	//! @param bytes ������ ������������ ������ � ������.
	//! @return ���������� ���������� ����.
	size_t WriteData(ulong64 fileOffset, const void* data, size_t bytes) const;

	//! ���������� ������ ����� ������ size.
	void SetSize(ulong64 size) const;

	//! ��������� ���� ������� � ������.
	//! @param fileName ���� � �����.
	//! @param[out] oFileOpened ����������, ��� �� ������ ����.
	//! @return ������ � ���������� ����� ����� ��� ������ ������, ���� ���� �� ��� ������.
	static String ReadAsString(StringView fileName, ErrorStatus& status);

	struct NativeHandle;

	//! ���������� ����� ����� ��.
	//! ��� Windows ������������ �������� ����� ��������� � HANDLE (�������� ����� CreateFile).
	//! ��� ��������� �� ������������ �������� ����� ��������� � int (file descriptor, �������� ����� open).
	//! �������� ������������� ������ ������� ������� � ��� ������ ���������, ���� � ���� ���� ��������.
	NativeHandle* GetNativeHandle() const {return mHandle;}

	//! ������ ������ OsFile �� ����������� ������ ��
	//! @param handle ����� �� ���� HANDLE (��� WinAPI) ��� int (file descriptor, ��� �� ����� Windows).
	//! @param owning ���� true, ���������� handle ����� ������ ��������� �������� �������������.
	static OsFile FromNative(NativeHandle* handle, bool owning);

	bool OwnsHandle() const
	{
		INTRA_DEBUG_ASSERT(!mOwning || mHandle != null);
		return mOwning;
	}

	StringView FullPath() const {return mFullPath;}

private:
	NativeHandle* mHandle;
	Mode mMode;
	bool mOwning;
	String mFullPath;
};

}}

INTRA_WARNING_POP
