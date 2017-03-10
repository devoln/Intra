#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "Range/Generators/StringView.h"
#include "Container/Sequential/String.h"
#include "FileMapping.h"

//TODO Это временно, пока не переведу все потоки на новые файлы
#ifndef INTRA_NO_FILE_LOGGING
#define INTRA_NO_FILE_LOGGING
#endif

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
class OsFile;

class OsFileSystem: public IFileSystem
{
public:
	OsFileSystem();
	OsFileSystem(const OsFileSystem&) = default;
	OsFileSystem& operator=(const OsFileSystem&) = default;

	//! Проверить существования файла fileName.
	bool FileExists(StringView fileName) const override final;

	//! Удалить файл fileName.
	//! Если файл не существует или его нельзя удалить, возвращает false.
	//! \return true, если операция была выполнена.
	bool FileDelete(StringView fileName) override final;

	//! Переместить и\или переименовать файл или директорию oldFilename в newFilename.
	//! Если файл уже существует, поведение функции зависит от параметра overwriteExisting.
	//! \return true, если операция была выполнена.
	bool FileMove(StringView oldFilename, StringView newFilename, bool overwriteExisting) override final;

	//! Возвращает информацию о файле fileName.
	FileInfo FileGetInfo(StringView fileName) const override final;

	//! Вовзаращает время последней модификации файла fileName.
	ulong64 FileGetTime(StringView filename) const override final;

	//! Вовзаращает время последней модификации файла fileName.
	ulong64 FileGetSize(StringView filename) const override final;

	//! Возвращает текущую директорию, относительно которой производятся все файловые операции при указании относительного пути.
	StringView CurrentDirectory() const override final {return mCurrentDirectory;}

	//! Установить текущую директорию.
	void SetDirectory(StringView newDir) override final {mCurrentDirectory = newDir;}

	//! Получить полный путь к файлу fileName.
	String GetFullFileName(StringView fileName) const;

	//! Отобразить область файла с именем fileName в память с доступом только для чтения.
	//! @param offset Начало отображаемой области.
	//! @param bytes Размер отображаемой области в байтах.
	FileMapping MapFile(StringView fileName, ulong64 offset, size_t bytes)
	{return FileMapping(GetFullFileName(fileName), offset, bytes);}
	
	//! Отобразить целиком файл с именем fileName в память с доступом только для чтения.
	FileMapping MapFile(StringView fileName)
	{return FileMapping(GetFullFileName(fileName));}

	//! Отобразить область файла с именем fileName в память с доступом для чтения и записи.
	//! @param offset Начало отображаемой области.
	//! @param bytes Размер отображаемой области в байтах.
	WritableFileMapping MapFileWrite(StringView fileName, ulong64 offset, size_t bytes)
	{return WritableFileMapping(GetFullFileName(fileName), offset, bytes);}
	
	//! Отобразить целиком файл с именем fileName в память с доступом для чтения и записи.
	WritableFileMapping MapFileWrite(StringView fileName)
	{return WritableFileMapping(GetFullFileName(fileName));}

	//! Открыть файл fileName только для чтения.
	//! Файл должен существовать, иначе вернёт null.
	OsFile FileOpen(StringView fileName);

	//! Открыть файл fileName только для записи.
	OsFile FileOpenWrite(StringView fileName);

	//! Открыть файл fileName для чтения и записи.
	OsFile FileOpenReadWrite(StringView fileName);

	String FileToString(StringView fileName);

private:
	String mCurrentDirectory;
};

extern OsFileSystem OS;

}}

INTRA_WARNING_POP
