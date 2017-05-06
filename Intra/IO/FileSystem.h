#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"
#include "Utils/StringView.h"
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
	virtual FileInfo FileGetInfo(StringView fileName) const = 0;
	virtual ulong64 FileGetTime(StringView filename) const = 0;
	virtual ulong64 FileGetSize(StringView filename) const = 0;

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
	FileInfo FileGetInfo(StringView fileName) const final;

	//! ����������� ����� ��������� ����������� ����� fileName.
	ulong64 FileGetTime(StringView filename) const final;

	//! ����������� ����� ��������� ����������� ����� fileName.
	ulong64 FileGetSize(StringView filename) const final;

	//! ���������� ������� ����������, ������������ ������� ������������ ��� �������� �������� ��� �������� �������������� ����.
	StringView CurrentDirectory() const final {return mCurrentDirectory;}

	//! ���������� ������� ����������.
	void SetDirectory(StringView newDir) final {mCurrentDirectory = newDir;}

	//! �������� ������ ���� � ����� fileName.
	String GetFullFileName(StringView fileName) const;

	//! ���������� ������� ����� � ������ fileName � ������ � �������� ������ ��� ������.
	//! @param offset ������ ������������ �������.
	//! @param bytes ������ ������������ ������� � ������.
	FileMapping MapFile(StringView fileName, ulong64 offset, size_t bytes)
	{return FileMapping(GetFullFileName(fileName), offset, bytes);}
	
	//! ���������� ������� ���� � ������ fileName � ������ � �������� ������ ��� ������.
	FileMapping MapFile(StringView fileName)
	{return FileMapping(GetFullFileName(fileName));}

	//! ���������� ������� ����� � ������ fileName � ������ � �������� ��� ������ � ������.
	//! @param offset ������ ������������ �������.
	//! @param bytes ������ ������������ ������� � ������.
	WritableFileMapping MapFileWrite(StringView fileName, ulong64 offset, size_t bytes)
	{return WritableFileMapping(GetFullFileName(fileName), offset, bytes);}
	
	//! ���������� ������� ���� � ������ fileName � ������ � �������� ��� ������ � ������.
	WritableFileMapping MapFileWrite(StringView fileName)
	{return WritableFileMapping(GetFullFileName(fileName));}

	//! ������� ���� fileName ��� ������.
	//! ���� ���� �� ���������� ��� �� ������ ������� �� ������ ��������, ����� null.
	FileReader FileOpen(StringView fileName);

	//! ������� ���� fileName ��� ������.
	//! ���� ���� ��� ����������, ������ ���������� ������ ������, ��� ���������� � ����.
	FileWriter FileOpenWrite(StringView fileName, ulong64 offset=0);

	//! ������� ���� fileName ��� ������. ���� ���� ��� ����������, �� ��� ���������� ����� �����.
	FileWriter FileOpenOverwrite(StringView fileName);

	//! ������� ���� fileName ��� ������ � �����.
	FileWriter FileOpenAppend(StringView fileName);

	//! ��������� ���� ������� � ������.
	String FileToString(StringView fileName);

private:
	String mCurrentDirectory;
};

extern OsFileSystem OS;

}}

INTRA_WARNING_POP
