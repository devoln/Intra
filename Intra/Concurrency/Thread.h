#pragma once

#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"

#include "Utils/Delegate.h"
#include "Utils/Unique.h"
#include "Utils/StringView.h"

#undef Yield

#define INTRA_LIBRARY_THREAD_None 0
#define INTRA_LIBRARY_THREAD_WinAPI 1
#define INTRA_LIBRARY_THREAD_Cpp11 2
#define INTRA_LIBRARY_THREAD_PThread 3

#ifndef INTRA_LIBRARY_THREAD

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_WinAPI

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

#ifdef __EMSCRIPTEN_PTHREADS__
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_PThread
#else
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_None
#endif

#else
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_PThread
#endif

#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Concurrency {

#if(INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None)
class Thread
{
public:
	struct Data;
	typedef Data* Handle;
private:
	Unique<Data> mHandle;
public:
	//! В Windows HANDLE.
	//! В остальных системах pthread_t*.
	typedef void* NativeHandle;

	typedef Delegate<void()> Func;

	Thread(null_t=null) noexcept;
	Thread(Thread&& rhs) noexcept;
	explicit Thread(Func func);
	explicit Thread(StringView name, Func func):
		Thread(Cpp::Move(func)) {SetName(name);}

	//! Деструктор вызывает Interrupt и Join.
	~Thread();
	Thread& operator=(Thread&& rhs);

	//! Блокировать выполнение текущего потока до тех пор,
	//! пока указанный поток не завершится, не истечёт указанный таймаут, либо не будет вызван Interrupt на этом потоке.
	//! @param timeout Таймаут в миллисекундах.
	//! @return Возвращает true, если в результате поток завершил своё выполнение.
	bool JoinMs(ulong64 timeout);

	//! Блокировать выполнение текущего потока до тех пор,
	//! пока не завершится указанный поток.
	//! @return Возвращает true, если в результате поток завершил своё выполнение.
	bool Join();

	//! Отсоединить поток от объекта.
	//! Текущий объект переходит в нулевое состояние,
	//! поток продолжает выполняться независимо от него.
	void Detach();

	//! Установить флаг прерывания потока.
	//! Если механизм прерывания потока не был отключён вызовом DisableInterruption(false)
	//! или через #define INTRA_THREAD_NO_FULL_INTERRUPT, то
	//! прерывает ожидание на Sleep, Join/JoinMs, CondVar::Wait/WaitMs и эти функции возвращают false.
	void Interrupt();

	//! Возвращает флаг прерывания потока.
	bool IsInterrupted() const;

#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	//! Разрешить методу Interrupt прерывать ожидание данного потока.
	forceinline void EnableInterruption() {allowInterruption(true);}
#endif
	//! Запретить методу Interrupt прерывать ожидание данного потока.
	forceinline void DisableInterruption()
	{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		allowInterruption(false);
#endif
	}

	//! Возвращает true, если для данного потока включена поддержка прерывания ожидания.
	bool IsInterruptionEnabled() const;

	//! Возвращает, выполняется ли поток в данный момент.
	bool IsRunning() const;

	//! Установить имя потока. Оно может отображаться в отладчике.
	void SetName(StringView name);

	//! Получить имя потока, установленное через SetName.
	StringView Name() const;

	//! @return Зависимый от платформы дескриптор потока:
	//! Для Windows HANDLE (созданный через _beginthreadex\CreateThread)
	//! Для остальных платформ pthread*.
	//! Полученный дескриптор нельзя использовать после Detach или уничтожения потока.
	NativeHandle GetNativeHandle() const;

private:
	Thread(const Thread& rhs) = delete;
	Thread& operator=(const Thread&) = delete;

#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	void allowInterruption(bool allow);
#endif
};

class SeparateCondVar;
class Mutex;

struct TThisThread
{
	constexpr forceinline TThisThread() {}
	TThisThread(const TThisThread&) = delete;
	TThisThread& operator=(const TThisThread&) = delete;

	//! Установить флаг потока, который говорит о том, что пора завершаться.
	//! Работает только для потоков, созданных с помощью класса Thread.
	static void Interrupt();

	static bool IsInterrupted();

	//! Дескриптор ОС текущего потока.
	static Thread::NativeHandle NativeHandle();


	static StringView Name();

	//! Отдаёт текущий квант времени ОС.
	static void Yield();

	static bool Sleep(ulong64 milliseconds);

	#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	//! Разрешить методу Interrupt прерывать ожидание данного потока.
	static forceinline void EnableInterruption() {allowInterruption(true);}
#endif
	//! Запретить методу Interrupt прерывать ожидание данного потока.
	static forceinline void DisableInterruption()
	{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		allowInterruption(false);
#endif
	}

	//! Возвращает true, если для данного потока включена поддержка прерывания ожидания.
	static bool IsInterruptionEnabled();

private:
#if !defined(INTRA_THREAD_NO_FULL_INTERRUPT) || defined(INTRA_DEBUG)
	friend class SeparateCondVar;
	static void onWait(SeparateCondVar* cv, Mutex* mutex);
#endif
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	static void allowInterruption(bool allow);
#endif

#ifdef INTRA_DEBUG
	static Thread::Handle getHandle();
#endif
};
extern const TThisThread ThisThread;
#else
class Thread;
struct TThisThread;
#endif

}
using Concurrency::Thread;

#if(INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None)
using Concurrency::ThisThread;
#endif

}

INTRA_WARNING_POP
