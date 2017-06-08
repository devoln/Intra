#include "Thread.h"
#include "Atomic.h"

#include "Cpp/Warnings.h"
#include "Cpp/PlatformDetect.h"
#include "Cpp/Runtime.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#include "detail/ThreadCommonWinAPI.hxx"
#endif

#include "detail/BasicThreadData.hxx"

#if(INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_Dummy)

#include "detail/ThreadDummy.hxx"

#elif(INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_CPPLIB)

#include "detail/ThreadCpp11.hxx"

#elif(INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_WinAPI)

#include "detail/ThreadWinAPI.hxx"

#elif(INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_PThread)

#include "detail/ThreadPThread.hxx"
#include "detail/ThreadSleep.hxx"

#endif



namespace Intra { namespace Concurrency {

Thread::Thread(Func func):
	mHandle(new Data(Cpp::Move(func)))
{
	if(mHandle->IsDetached)
		mHandle = null;
}

Thread::Thread(null_t) noexcept {}

Thread::~Thread() {}

Thread& Thread::operator=(Thread&&) = default;

bool Thread::Join()
{return mHandle == null || mHandle->Join();}

bool Thread::Join(uint timeoutMs)
{return mHandle == null || mHandle->Join(timeoutMs);}

void Thread::Interrupt()
{
	if(mHandle == null) return;
	mHandle->Interrupt();
}

bool Thread::IsInterrupted() const
{return mHandle != null && mHandle->IsInterrupted;}

bool Thread::IsRunning() const
{return mHandle != null && mHandle->IsRunning;}

void Thread::SetName(StringView name)
{
	if(mHandle == null) return;
	mHandle->Name = name;
	mHandle->SetName();
}

StringView Thread::GetName() const
{return mHandle == null? null: mHandle->Name;}

void Thread::Detach()
{
	if(mHandle == null) return;
	mHandle.Release()->Detach();
}

Thread::NativeHandle Thread::GetNativeHandle() const
{
	return mHandle == null? null:
		mHandle->GetNativeHandle();
}

Thread::Handle ThisThread::Handle()
{return Thread::Data::Current;}

void ThisThread::Interrupt()
{
	if(Thread::Data::Current == null) return;
	Thread::Data::Current->Interrupt();
}

bool ThisThread::IsInterrupted()
{
	return Thread::Data::Current == null? false:
		Thread::Data::Current->IsInterrupted.Load();
}

StringView ThisThread::Name()
{
	return Thread::Data::Current == null? null:
		Thread::Data::Current->Name;
}

}}

INTRA_WARNING_POP
