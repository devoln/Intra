#pragma once

#include "Platform/FundamentalTypes.h"
#include "Range/Generators/StringView.h"
#include "Container/ForwardDecls.h"

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
	virtual bool FileExists(StringView fileName) const = 0;
	virtual bool FileDelete(StringView filename) = 0;
	virtual bool FileMove(StringView oldFilename, StringView newFilename, bool overwriteExisting) = 0;
	virtual FileInfo FileGetInfo(StringView fileName) const = 0;
	virtual ulong64 FileGetTime(StringView filename) const = 0;

	virtual StringView CurrentDirectory() const = 0;
	virtual void SetDirectory(StringView newDir) = 0;
};

class FileMapping;

class OsFileSystem: public IFileSystem
{
public:
	OsFileSystem();
	OsFileSystem(const OsFileSystem&) = default;
	OsFileSystem& operator=(const OsFileSystem&) = default;

	//! ��������� ������������� ����� fileName.
	bool FileExists(StringView fileName) const override final;

	//! ������� ���� fileName.
	//! ���� ���� �� ���������� ��� ��� ������ �������, ���������� false.
	//! \return true, ���� �������� ���� ���������.
	bool FileDelete(StringView fileName) override final;

	//! ����������� �\��� ������������� ���� ��� ���������� oldFilename � newFilename.
	//! ���� ���� ��� ����������, ��������� ������� ������� �� ��������� overwriteExisting.
	//! \return true, ���� �������� ���� ���������.
	bool FileMove(StringView oldFilename, StringView newFilename, bool overwriteExisting) override final;

	//! ���������� ���������� � ����� fileName.
	FileInfo FileGetInfo(StringView fileName) const override final;

	//! ����������� ����� ��������� ����������� ����� fileName.
	ulong64 FileGetTime(StringView filename) const override final;

	//! ����������� ����� ��������� ����������� ����� fileName.
	ulong64 FileGetSize(StringView filename) const override final;

	//! ���������� ������� ����������, ������������ ������� ������������ ��� �������� �������� ��� �������� �������������� ����.
	StringView CurrentDirectory() const override final {return mCurrentDirectory;}

	//! ���������� ������� ����������.
	void SetDirectory(StringView newDir) override final {mCurrentDirectory = newDir;}

	//! �������� ������ ���� � ����� fileName.
	String GetFullFileName(StringView fileName) const;

private:
	String mCurrentDirectory;
};

extern OsFileSystem OS;

}}
