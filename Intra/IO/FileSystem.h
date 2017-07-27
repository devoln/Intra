﻿#pragma once

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
	bool Exist() const {return Size != 0 || LastModified != 0;}

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

	//! Проверить существования файла fileName.
	bool FileExists(StringView fileName) const final;

	//! Удалить файл fileName.
	//! Если файл не существует или его нельзя удалить, возвращает false.
	//! \return true, если операция была выполнена.
	bool FileDelete(StringView fileName) final;

	//! Переместить и\или переименовать файл или директорию oldFilename в newFilename.
	//! Если файл уже существует, поведение функции зависит от параметра overwriteExisting.
	//! \return true, если операция была выполнена.
	bool FileMove(StringView oldFilename, StringView newFilename, bool overwriteExisting) final;

	//! Возвращает информацию о файле fileName.
	FileInfo FileGetInfo(StringView fileName, ErrorStatus& status) const final;

	//! Вовзаращает время последней модификации файла fileName.
	ulong64 FileGetTime(StringView filename, ErrorStatus& status) const final;

	//! Вовзаращает время последней модификации файла fileName.
	ulong64 FileGetSize(StringView filename, ErrorStatus& status) const final;

	//! Возвращает текущую директорию, относительно которой производятся все файловые операции при указании относительного пути.
	StringView CurrentDirectory() const final {return mCurrentDirectory;}

	//! Установить текущую директорию.
	void SetDirectory(StringView newDir) final {mCurrentDirectory = newDir;}

	//! Получить полный путь к файлу fileName.
	String GetFullFileName(StringView fileName) const;

	//! Отобразить область файла с именем fileName в память с доступом только для чтения.
	//! @param offset Начало отображаемой области.
	//! @param bytes Размер отображаемой области в байтах.
	FileMapping MapFile(StringView fileName, ulong64 offset, size_t bytes, ErrorStatus& status)
	{return FileMapping(GetFullFileName(fileName), offset, bytes, status);}
	
	//! Отобразить целиком файл с именем fileName в память с доступом только для чтения.
	FileMapping MapFile(StringView fileName, ErrorStatus& status)
	{return FileMapping(GetFullFileName(fileName), status);}

	//! Отобразить область файла с именем fileName в память с доступом для чтения и записи.
	//! @param offset Начало отображаемой области.
	//! @param bytes Размер отображаемой области в байтах.
	WritableFileMapping MapFileWrite(StringView fileName, ulong64 offset, size_t bytes, ErrorStatus& status)
	{return WritableFileMapping(GetFullFileName(fileName), offset, bytes, status);}
	
	//! Отобразить целиком файл с именем fileName в память с доступом для чтения и записи.
	WritableFileMapping MapFileWrite(StringView fileName, ErrorStatus& status)
	{return WritableFileMapping(GetFullFileName(fileName), status);}

	//! Открыть файл fileName для чтения.
	//! Если файл не существует или не удаётся открыть по другим причинам, вернёт null.
	FileReader FileOpen(StringView fileName, ErrorStatus& status);

	//! Открыть файл fileName для записи.
	//! Если файл уже существует, запись происходит поверх данных, уже записанных в файл.
	FileWriter FileOpenWrite(StringView fileName, ulong64 offset, ErrorStatus& status);
	FileWriter FileOpenWrite(StringView fileName, ErrorStatus& status);

	//! Открыть файл fileName для записи. Если файл уже существует, всё его содержимое будет стёрто.
	FileWriter FileOpenOverwrite(StringView fileName, ErrorStatus& status);

	//! Открыть файл fileName для записи в конец.
	FileWriter FileOpenAppend(StringView fileName, ErrorStatus& status);

	//! Прочитать файл целиком в строку.
	String FileToString(StringView fileName, ErrorStatus& status);

private:
	String mCurrentDirectory;
};

extern OsFileSystem OS;

}}

INTRA_WARNING_POP
