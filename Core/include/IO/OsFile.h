#pragma once

#include "Algo/Search/Single.h"
#include "Algo/Raw/Read.h"
#include "Range/Generators/StringView.h"
#include "Range/Generators/ArrayRange.h"
#include "Algo/Mutation/Fill.h"
#include "Container/Sequential/String.h"
#include "Platform/Debug.h"
#include "Platform/CppWarnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

class OsFile
{
public:
	enum class Mode {Read, Write, ReadWrite, None};

	OsFile(StringView fileName, Mode mode, bool disableSystemBuffering=false);
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
	static String ReadAsString(StringView fileName, bool& oFileOpened);

	//! ��������� ���� ������� � ������.
	//! @param fileName ���� � �����.
	//! @return ������ � ���������� ����� ����� ��� ������ ������, ���� ���� �� ��� ������.
	static String ReadAsString(StringView fileName) {bool unused; return ReadAsString(fileName, unused);}

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