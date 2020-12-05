#pragma once

#include "Intra/Range/StringView.h"
#include "Intra/Range/Span.h"
#include "Intra/Assert.h"
#include "IntraX/System/Error.h"

#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Stream/RawRead.h"
#include "Intra/Range/Mutation/Fill.h"

#include "IntraX/Container/Sequential/String.h"

INTRA_BEGIN
class OsFile
{
public:
	enum class Mode {Read, Write, ReadWrite, None};

	OsFile(StringView fileName, Mode mode, bool disableSystemBuffering, ErrorReporter err);
	OsFile(StringView fileName, Mode mode, ErrorReporter err): OsFile(fileName, mode, false, err) {}
	OsFile(decltype(null)=null): mHandle(null), mMode(Mode::None), mOwning(false) {}
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

	bool operator==(decltype(null)) const {return mHandle == null;}
	bool operator!=(decltype(null)) const {return mHandle != null;}

	INTRA_FORCEINLINE OsFile& operator=(decltype(null)) {Close(); return *this;}

	/// ������� ����, ��������������� � ���� ��������.
	/// ������� ������ ��������� � null ���������.
	void Close();

	/// ��������� ������ �� ����� �� ���������� ��������.
	/// @param fileOffset �������� ������ ����������� ������ � �����.
	/// @param data ��������� �� ������ ������������ �� ����� ����� ������.
	/// @param bytes ������ ����������� ������ � ������.
	/// @return ���������� ����������� ����.
	size_t ReadData(uint64 fileOffset, void* data, size_t bytes, ErrorReporter err) const;
	
	/// ���������� ������ ����� ��� 0, ���� ��� �� ����.
	uint64 Size(ErrorReporter err = IgnoreErrors) const;

	/// �������� ������ � ���� �� ���������� ��������.
	/// @param fileOffset �������� ������ ������������ ������ � �����.
	/// @param data ��������� �� ������ ������������� � ���� ����� ������.
	/// @param bytes ������ ������������ ������ � ������.
	/// @return ���������� ���������� ����.
	size_t WriteData(uint64 fileOffset, const void* data, size_t bytes, ErrorReporter err) const;

	/// ���������� ������ ����� ������ size.
	void SetSize(uint64 size, ErrorReporter err) const;

	/// ��������� ���� ������� � ������.
	/// @param fileName ���� � �����.
	/// @param[out] oFileOpened ����������, ��� �� ������ ����.
	/// @return ������ � ���������� ����� ����� ��� ������ ������, ���� ���� �� ��� ������.
	static String ReadAsString(StringView fileName, ErrorReporter err);

	struct NativeData;
	typedef NativeData* NativeHandle;

	/// ���������� ����� ����� ��.
	/// ��� Windows ������������ �������� ����� ��������� � HANDLE (�������� ����� CreateFile).
	/// ��� ��������� �� ������������ �������� ����� ��������� � int (file descriptor, �������� ����� open).
	/// �������� ������������� ������ ������� ������� � ��� ������ ���������, ���� � ���� ���� ��������.
	NativeHandle GetNativeHandle() const {return mHandle;}

	/// ������ ������ OsFile �� ����������� ������ ��
	/// @param handle ����� �� ���� HANDLE (��� WinAPI) ��� int (file descriptor, ��� �� ����� Windows).
	/// @param owning ���� true, ���������� handle ����� ������ ��������� �������� �������������.
	static OsFile FromNative(NativeHandle handle, bool owning);

	bool OwnsHandle() const
	{
		INTRA_PRECONDITION(!mOwning || mHandle != null);
		return mOwning;
	}

	StringView FullPath() const {return mFullPath;}

private:
	NativeHandle mHandle;
	Mode mMode;
	bool mOwning;
	String mFullPath;
};
INTRA_END
