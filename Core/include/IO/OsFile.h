#pragma once

#include "Algo/Search/Single.h"
#include "Algo/Raw/Read.h"
#include "Range/Generators/StringView.h"
#include "Range/Generators/ArrayRange.h"
#include "Algo/Mutation/Fill.h"
#include "Container/Sequential/String.h"
#include "Platform/Debug.h"

namespace Intra { namespace IO {

class FileReader;
class FileWriter;

class OsFile
{
public:
	enum class Mode {Read, Write, ReadWrite, None};

	OsFile(StringView fileName, Mode mode, bool disableSystemBuffering=false);
	OsFile(null_t=null): mHandle(null), mMode(Mode::None), mOwning(false) {}
	~OsFile() {Close();}
	
	OsFile(OsFile&& rhs):
		mHandle(rhs.mHandle), mOwning(rhs.mOwning)
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

	OsFile& operator=(null_t) {Close(); return *this;}

	//! Закрыть файл, ассоциированный с этим объектом.
	//! Текущий объект переходит в null состояние.
	void Close();

	//! Прочитать данные из файла по указанному смещению.
	//! @param fileOffset Смещение начала считываемых данных в файле.
	//! @param data Указатель на начало считываемого из файла блока памяти.
	//! @param bytes Размер считываемых данных в байтах.
	//! @return Количество прочитанных байт.
	size_t ReadData(ulong64 fileOffset, void* data, size_t bytes) const;
	
	//! Возвращает размер файла или 0, если это не файл.
	ulong64 Size() const;

	//! Записать данные в файл по указанному смещению.
	//! @param fileOffset Смещение начала записываемых данных в файле.
	//! @param data Указатель на начало записываемого в файл блока памяти.
	//! @param bytes Размер записываемых данных в байтах.
	//! @return Количество записанных байт.
	size_t WriteData(ulong64 fileOffset, const void* data, size_t bytes) const;

	//! Установить размер файла равным size.
	void SetSize(ulong64 size) const;

	//! Возвращает поток для чтения файла.
	FileReader Reader() const;

	//! Возвращает поток для записи файла, начиная с позиции startOffset.
	FileWriter Writer(ulong64 startOffset=0) const;

	//! Возвращает поток для записи файла в конец.
	FileWriter Appender() const;

	//! Возвращает поток для чтения файла.
	FileReader AsRange() const;

	//! Прочитать файл целиком в строку.
	//! @param fileName Путь к файлу.
	//! @param[out] oFileOpened Возвращает, был ли открыт файл.
	//! @return Строка с содержимым всего файла или пустая строка, если файл не был открыт.
	static String ReadAsString(StringView fileName, bool& oFileOpened);

	//! Прочитать файл целиком в строку.
	//! @param fileName Путь к файлу.
	//! @return Строка с содержимым всего файла или пустая строка, если файл не был открыт.
	static String ReadAsString(StringView fileName) {bool unused; return ReadAsString(fileName, unused);}

	struct NativeHandle;

	//! Возвращает хендл файла ОС.
	//! Для Windows возвращаемое значение нужно приводить к HANDLE (создаётся через CreateFile).
	//! Для остальных ОС возвращаемое значение нужно приводить к int (file descriptor, создаётся через open).
	//! Владелец возвращаемого хендла остаётся прежним и его нельзя закрывать, пока у него есть владелец.
	NativeHandle* GetNativeHandle() const {return mHandle;}

	//! Создаёт объект OsFile из переданного хендла ОС
	//! @param handle Хендл ОС типа HANDLE (для WinAPI) или int (file descriptor, для ОС кроме Windows).
	//! @param owning Если true, переданный handle будет закрыт созданным объектом автоматически.
	static OsFile FromNative(NativeHandle* handle, bool owning);

	bool OwnsHandle() const
	{
		INTRA_DEBUG_ASSERT(!mOwning || mHandle != null);
		return mOwning;
	}

private:
	NativeHandle* mHandle;
	Mode mMode;
	bool mOwning;
};

}}
