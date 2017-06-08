#include "Concurrency/Thread.h"
#include "BasicThreadData.hxx"

namespace Intra { namespace Concurrency {

struct Thread::Data: detail::BasicThreadData
{
	Data(Func func)
	{
		func();
		IsDetached = true;
	}

	bool Join(uint = 0) {return true;}
	void Detach() {}
	NativeHandle GetNativeHandle() {return null;}
};

}}
