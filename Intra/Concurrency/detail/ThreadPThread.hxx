#include "Concurrency/Thread.h"
#include "BasicThreadData.hxx"

#include "System/Stopwatch.h"

#include <pthread.h>
#include <sched.h>

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_FreeBSD)
#include <pthread_np.h>
#endif

#if(INTRA_PLATFORM_OS != INTRA_PLATFORM_OS_Windows)
#include <unistd.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "pthread.lib")
#endif

namespace Intra { namespace Concurrency {

struct Thread::Data: detail::BasicThreadData
{
	enum: size_t {ThreadStackSize = 1048576};

	pthread_t Thread;

	Data(Func func)
	{
		Function = Cpp::Move(func);

		pthread_attr_t attribute;
		pthread_attr_init(&attribute);
		pthread_attr_setstacksize(&attribute, ThreadStackSize);
		if(pthread_create(&Thread, &attribute, ThreadProc, this) != 0)
		{
			IsDetached = true;
			Thread = 0;
		}
	}

	static void* ThreadProc(void* lpParam)
	{
		Handle(lpParam)->ThreadFunc();
		return null;
	}

	~Data()
	{
		if(IsDetached) return;
		Interrupt();
		Join();
	}

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_MacOS || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Android)
	struct waitData
	{
		pthread_t waitID;
		int done;
	};

	static void* join_timeout_helper(void* arg)
	{
		waitData* data = static_cast<waitData*>(arg);
		pthread_join(data->waitID, null);
		data->done = true;
		return null;
	}

	bool Join(uint timeoutMs)
	{
		Stopwatch tim;
		pthread_t id;
		waitData data = {wid, false};
		if(pthread_create(&id, null, join_timeout_helper, &data) != 0) return -1;
	
		while(!data.done && tim.GetTimeMs() < timeoutNs)
		{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
			Sleep(10);
#else
			usleep(10000);
#endif
		}
		if(!data.done) pthread_cancel(id);
		pthread_join(id, null);
		if(data.done) return Join();
		return !IsRunning;
	}

#else

	bool Join(uint timeoutMs)
	{
		if(Thread == 0) return true;
		timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeoutMs/1000;
		ts.tv_nsec += (timeoutMs - timeoutMs/1000)*1000000;
		if(ts.tv_nsec >= 1000000000)
		{
			ts.tv_nsec -= 1000000000;
			ts.tv_sec++;
		}
		if(pthread_timedjoin_np(Thread, null, &ts) == 0)
		{
			Thread = 0;
			IsDetached = true;
			return true;
		}
		return false;
	}

#endif

	bool Join()
	{
		if(Thread == 0) return true;
		if(pthread_join(Thread, null) == 0)
		{
			Thread = 0;
			IsDetached = true;
		}
		return !IsRunning;
	}

	void Detach()
	{
		if(IsDetached) return;
		pthread_detach(Thread);
		INTRA_SYNCHRONIZED_BLOCK(GlobalThreadMutex)
		{
			if(!IsRunning) delete this;
			else IsDetached = true;
		}
	}

	void SetName()
	{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux)
		pthread_setname_np(Thread, String(Name.Take(15)).CStr());
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_FreeBSD)
		pthread_set_name_np(Thread, String(Name.Take(15)).CStr());
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_Windows)
		pthread_setname_np(Thread, Name.CStr());
#endif
	}

	NativeHandle GetNativeHandle()
	{return NativeHandle(Thread);}
};

void ThisThread::Yield() {sched_yield();}

}}
