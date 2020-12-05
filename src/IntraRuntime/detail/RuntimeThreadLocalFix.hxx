#if defined(__FreeBSD__)
#include "IntraX/Concurrency/Thread.h"

#if(INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_PThread)
#include <pthread.h>

// Workaround for thread_local keyword support on FreeBSD.
// Without this using thread_local objects on FreeBSD causes a linker error.

namespace detail {

struct ThreadLocalStorage
{
	ThreadLocalStorage() {pthread_key_create(&Key, ThreadDataDestructor);}
	~ThreadLocalStorage() {pthread_key_delete(Key);}

	pthread_key_t Key;

	struct DestructorNode
	{
		DestructorNode* Prev;
		void(*Destructor)(void*);
		void* Obj;
	};

	static void ThreadDataDestructor(void* fdn)
	{
		auto destructorNode = static_cast<DestructorNode*>(fdn);
		while(destructorNode)
		{
			destructorNode->Destructor(destructorNode->Obj);
			const auto prev = destructorNode->Prev;
			delete destructorNode;
			destructorNode = prev;
		}
	}

	static ThreadLocalStorage Instance;
};
ThreadLocalStorage ThreadLocalStorage::Instance;

}

extern "C" int __cxa_thread_atexit(void(*destructor)(void*), void* obj, void* dsoSymbol)
{
	(void)dsoSymbol;
	auto destructorNode = static_cast<detail::ThreadLocalStorage::DestructorNode*>(pthread_getspecific(detail::ThreadLocalStorage::Instance.Key));
	auto newDestructorNode = new detail::ThreadLocalStorage::DestructorNode{destructorNode, destructor, obj};
	pthread_setspecific(detail::ThreadLocalStorage::Instance.Key, newDestructorNode);
	return 0;
}

#endif

#endif

#ifdef __EMSCRIPTEN__

// Implentation of support for thread_local keyword on Emscripten.
// As browsers currently don't support multithreading, it is defined as no-op.
// Without this it would be a linker error.
extern "C" int __cxa_thread_atexit(void(*func)(), void* obj, void* dsoSymbol)
{
	(void)func;
	(void)obj;
	(void)dsoSymbol;
	return 0;
}

#endif
