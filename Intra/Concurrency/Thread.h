#pragma once

#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"

#include "Utils/Delegate.h"
#include "Utils/Unique.h"
#include "Utils/StringView.h"

#undef Yield

#define INTRA_LIBRARY_THREAD_Dummy 0
#define INTRA_LIBRARY_THREAD_WinAPI 1
#define INTRA_LIBRARY_THREAD_CPPLIB 2
#define INTRA_LIBRARY_THREAD_PThread 3

#ifndef INTRA_LIBRARY_THREAD

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_WinAPI

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

#ifdef __EMSCRIPTEN_PTHREADS__
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_PThread
#else
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_Dummy
#endif

#else
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_PThread
#endif

#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Concurrency {

class Thread
{
public:
	struct Data;
	typedef Data* Handle;
private:
	Unique<Data> mHandle;
	struct NativeData;
public:
	typedef NativeData* NativeHandle;

	typedef Delegate<void()> Func;

	Thread(null_t=null) noexcept;
	Thread(Thread&& rhs) = default;
	Thread(Func func);
	Thread(Func func, StringView name):
		Thread(Cpp::Move(func)) {SetName(name);}

	//! Деструктор вызывает Interrupt и Join.
	~Thread();
	Thread& operator=(Thread&& rhs);

	//! Блокировать выполнение текущего потока до тех пор,
	//! пока указанный поток не завершится или не истечёт указанный таймаут.
	//! @param timeoutInMs Таймаут в миллисекундах.
	//! @return Возвращает true, если в результате поток завершил своё выполнение.
	bool Join(uint timeoutInMs);

	//! Блокировать выполнение текущего потока до тех пор,
	//! пока не завершится указанный поток.
	//! @return Возвращает true, если в результате поток завершил своё выполнение.
	bool Join();

	//! Отсоединить поток от объекта.
	//! Текущий объект переходит в нулевое состояние,
	//! поток продолжает выполняться независимо от него.
	void Detach();

	//! Установить флаг потока, который говорит о том, что пора завершаться.
	void Interrupt();

	bool IsInterrupted() const;

	//! Возвращает, выполняется ли поток в данный момент.
	bool IsRunning() const;

	void SetName(StringView name);
	StringView GetName() const;

	Handle GetHandle() const {return mHandle.get();}

	//! @return Зависимый от платформы дескриптор потока:
	//! Для Windows HANDLE (созданный через _beginthreadex\CreateThread)
	//! Для остальных платформ pthread*.
	//! Полученный дескриптор нельзя использовать после Detach или уничтожения потока.
	NativeHandle GetNativeHandle() const;

private:
	Thread(const Thread& rhs) = delete;
	Thread& operator=(const Thread&) = delete;
};

struct ThisThread
{
	//! Установить флаг потока, который говорит о том, что пора завершаться.
	//! Работает только для потоков, созданных с помощью класса Thread.
	static void Interrupt();

	static bool IsInterrupted();

	//! Идентификатор текущего потока, созданного с помощью класса Thread.
	//! Для остальных потоков вернёт null.
	static Thread::Handle Handle();

	static StringView Name();

	//! Отдаёт текущий квант времени ОС.
	static void Yield();

	static bool Sleep(ulong64 milliseconds);
};

}
using Concurrency::Thread;
using Concurrency::ThisThread;

}

INTRA_WARNING_POP
