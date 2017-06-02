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

	//! Прочитать файл целиком в строку.
	//! @param fileName Путь к файлу.
	//! @param[out] oFileOpened Возвращает, был ли открыт файл.
	//! @return Строка с содержимым всего файла или пустая строка, если файл не был открыт.
	static String ReadAsString(StringView fileName, ErrorStatus& status);

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

	StringView FullPath() const {return mFullPath;}

private:
	NativeHandle* mHandle;
	Mode mMode;
	bool mOwning;
	String mFullPath;
};

}}

INTRA_WARNING_POP
