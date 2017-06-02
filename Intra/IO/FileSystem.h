#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"

#include "Utils/StringView.h"
#include "Utils/ErrorStatus.h"

#include "Container/Sequential/String.h"

#include "FileMapping.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

struct FileInfo
{
	bool Exist() const {return Size!=0 || LastModified!=0;}

	ulong64 Size;
	ulong64 LastModified;
};

class IFileSystem
{
public:
	virtual ~IFileSystem() {}

	virtual bool FileExists(StringView fileName) const = 0;
	virtual bool FileDelete(StringView filename) = 0;
	virtual bool FileMove(StringView oldFilename, StringView newFilename, bool overwriteExisting) = 0;
	virtual FileInfo FileGetInfo(StringView fileName, ErrorStatus& status) const = 0;
	virtual ulong64 FileGetTime(StringView filename, ErrorStatus& status) const = 0;
	virtual ulong64 FileGetSize(StringView filename, ErrorStatus& status) const = 0;

	virtual StringView CurrentDirectory() const = 0;
	virtual void SetDirectory(StringView newDir) = 0;
};

class FileMapping;
class FileReader;
class FileWriter;

class OsFileSystem: public IFileSystem
{
public:
	OsFileSystem();

	//! ��������� ������������� ����� fileName.
	bool FileExists(StringView fileName) const final;

	//! ������� ���� fileName.
	//! ���� ���� �� ���������� ��� ��� ������ �������, ���������� false.
	//! \return true, ���� �������� ���� ���������.
	bool FileDelete(StringView fileName) final;

	//! ����������� �\��� ������������� ���� ��� ���������� oldFilename � newFilename.
	//! ���� ���� ��� ����������, ��������� ������� ������� �� ��������� overwriteExisting.
	//! \return true, ���� �������� ���� ���������.
	bool FileMove(StringView oldFilename, StringView newFilename, bool overwriteExisting) final;

	//! ���������� ���������� � ����� fileName.
	FileInfo FileGetInfo(StringView fileName, ErrorStatus& status) const final;

	//! ����������� ����� ��������� ����������� ����� fileName.
	ulong64 FileGetTime(StringView filename, ErrorStatus& status) const final;

	//! ����������� ����� ��������� ����������� ����� fileName.
	ulong64 FileGetSize(StringView filename, ErrorStatus& status) const final;

	//! ���������� ������� ����������, ������������ ������� ������������ ��� �������� �������� ��� �������� �������������� ����.
	StringView CurrentDirectory() const final {return mCurrentDirectory;}

	//! ���������� ������� ����������.
	void SetDirectory(StringView newDir) final {mCurrentDirectory = newDir;}

	//! �������� ������ ���� � ����� fileName.
	String GetFullFileName(StringView fileName) const;

	//! ���������� ������� ����� � ������ fileName � ������ � �������� ������ ��� ������.
	//! @param offset ������ ������������ �������.
	//! @param bytes ������ ������������ ������� � ������.
	FileMapping MapFile(StringView fileName, ulong64 offset, size_t bytes, ErrorStatus& status)
	{return FileMapping(GetFullFileName(fileName), offset, bytes, status);}
	
	//! ���������� ������� ���� � ������ fileName � ������ � �������� ������ ��� ������.
	FileMapping MapFile(StringView fileName, ErrorStatus& status)
	{return FileMapping(GetFullFileName(fileName), status);}

	//! ���������� ������� ����� � ������ fileName � ������ � �������� ��� ������ � ������.
	//! @param offset ������ ������������ �������.
	//! @param bytes ������ ������������ ������� � ������.
	WritableFileMapping MapFileWrite(StringView fileName, ulong64 offset, size_t bytes, ErrorStatus& status)
	{return WritableFileMapping(GetFullFileName(fileName), offset, bytes, status);}
	
	//! ���������� ������� ���� � ������ fileName � ������ � �������� ��� ������ � ������.
	WritableFileMapping MapFileWrite(StringView fileName, ErrorStatus& status)
	{return WritableFileMapping(GetFullFileName(fileName), status);}

	//! ������� ���� fileName ��� ������.
	//! ���� ���� �� ���������� ��� �� ������ ������� �� ������ ��������, ����� null.
	FileReader FileOpen(StringView fileName, ErrorStatus& status);

	//! ������� ���� fileName ��� ������.
	//! ���� ���� ��� ����������, ������ ���������� ������ ������, ��� ���������� � ����.
	FileWriter FileOpenWrite(StringView fileName, ulong64 offset, ErrorStatus& status);
	FileWriter FileOpenWrite(StringView fileName, ErrorStatus& status);

	//! ������� ���� fileName ��� ������. ���� ���� ��� ����������, �� ��� ���������� ����� �����.
	FileWriter FileOpenOverwrite(StringView fileName, ErrorStatus& status);

	//! ������� ���� fileName ��� ������ � �����.
	FileWriter FileOpenAppend(StringView fileName, ErrorStatus& status);

	//! ��������� ���� ������� � ������.
	String FileToString(StringView fileName, ErrorStatus& status);

private:
	String mCurrentDirectory;
};

extern OsFileSystem OS;

}}

INTRA_WARNING_POP
