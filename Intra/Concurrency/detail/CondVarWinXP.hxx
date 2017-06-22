#pragma once

//This file contains adapted from Chromium project implementation of condition variables for Windows XP.

#include "Concurrency/Thread.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/Lock.h"

#include "Container/Sequential/Array.h"
#include "Container/Sequential/String.h"

#include "CondVarWinAPI.hxx"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace Intra { namespace Concurrency {

struct XPCondVar: BasicCondVarData
{
	XPCondVar() {}

	XPCondVar(const XPCondVar&) = delete;
	XPCondVar& operator=(const XPCondVar&) = delete;
	
	~XPCondVar()
	{
		INTRA_SYNCHRONIZED(internalMutex)
		{
			mRunState = SHUTDOWN; // Prevent any more waiting.

			INTRA_DEBUG_ASSERT_EQUALS(mRecyclingListSize, mAllocationCounter);
			if(mRecyclingListSize != mAllocationCounter) // Rare shutdown problem.
			{
				// There are threads of execution still in this->TimedWait() and yet the
				// caller has instigated the destruction of this instance :-/.
				// A common reason for such "overly hasty" destruction is that the caller
				// was not willing to wait for all the threads to terminate.  Such hasty
				// actions are a violation of our usage contract, but we'll give the
				// waiting thread(s) one last chance to exit gracefully (prior to our
				// destruction).
				// Note: mWaitingList *might* be empty, but recycling is still pending.
				internalMutex.Unlock();
				NotifyAll();  // Make sure all waiting threads have been signaled.
				Sleep(10);  // Give threads a chance to grab internalMutex.
							// All contained threads should be blocked on user_lock_ by now :-).
				internalMutex.Lock();
			}

			INTRA_DEBUG_ASSERT_EQUALS(mRecyclingListSize, mAllocationCounter);
		}
	}

	bool Wait(Mutex& mutex) final
	{
		// Default to "wait forever" timing, which means have to get a Signal()
		// or Broadcast() to come out of this wait state.
		return Wait(mutex, INFINITE);
	}

	bool Wait(Mutex& mutex, ulong64 timeoutMs) final
	{
		//base::ThreadRestrictions::AssertWaitAllowed();
		Event* waitingEvent = null;
		HANDLE handle = null;
		INTRA_SYNCHRONIZED(internalMutex)
		{
			if(mRunState != RUNNING) return false;  // Destruction in progress.
			waitingEvent = GetEventForWaiting();
			handle = waitingEvent->handle();
			INTRA_DEBUG_ASSERT(handle != null);
		}

		{
			mutex.Unlock();
			bool res = false;
			while(timeoutMs > INFINITE - 1)
			{
				res = WaitForSingleObject(handle, INFINITE - 1) == WAIT_OBJECT_0;
				if(res) break;
				timeoutMs -= INFINITE - 1;
			}
			if(!res) res = WaitForSingleObject(handle, DWORD(timeoutMs)) == WAIT_OBJECT_0;
			// Minimize spurious signal creation window by recycling asap.
			INTRA_SYNCHRONIZED(internalMutex)
			{
				RecycleEvent(waitingEvent);
			}
			mutex.Lock();
			return res;
		}  // Reacquire callers lock to depth at entry.
	}

	// Notify() will select one of the waiting threads, and signal it (signal its
	// cv_event).  For better performance we signal the thread that went to sleep
	// most recently (LIFO).  If we want fairness, then we wake the thread that has
	// been sleeping the longest (FIFO).
	void Notify() final
	{
		HANDLE handle = null;
		INTRA_SYNCHRONIZED(internalMutex)
		{
			if(mWaitingList.IsEmpty()) return;  // No one to signal.
												// Only performance option should be used.
												// This is not a leak from waiting_list.  See FAQ-question 12.
			handle = mWaitingList.PopBack()->handle();  // LIFO.
		}
		SetEvent(handle);
	}

	// NotifyAll() is guaranteed to signal all threads that were waiting (i.e., had
	// a cv_event internally allocated for them) before Broadcast() was called.
	void NotifyAll() final
	{
		Array<HANDLE> handles;  // See FAQ-question-10.
		INTRA_SYNCHRONIZED(internalMutex)
		{
			if(mWaitingList.IsEmpty()) return;
			while(!mWaitingList.IsEmpty())
				// This is not a leak from mWaitingList.  See FAQ-question 12.
				handles.AddLast(mWaitingList.PopBack()->handle());
		}
		while(!handles.Empty())
			SetEvent(handles.PopLastElement());
	}

	// Define Event class that is used to form circularly linked lists.
	// The list container is an element with NULL as its mHandle value.
	// The actual list elements have a non-zero mHandle value.
	// All calls to methods MUST be done under protection of a lock so that links
	// can be validated.  Without the lock, some links might asynchronously
	// change, and the assertions would fail (as would list change operations).
	class Event
	{
	public:
		// Default constructor with no arguments creates a list container.
		Event();
		~Event();

		// InitListElement transitions an instance from a container, to an element.
		void InitListElement();

		// Methods for use on lists.
		bool IsEmpty() const;
		void PushBack(Event* other);
		Event* PopFront();
		Event* PopBack();

		// Methods for use on list elements.
		// Accessor method.
		HANDLE handle() const;
		// Pull an element from a list (if it's in one).
		Event* Extract();

		// Method for use on a list element or on a list.
		bool IsSingleton() const;

	   private:
		// Provide pre/post conditions to validate correct manipulations.
		bool ValidateAsDistinct(Event* other) const;
		bool ValidateAsItem() const;
		bool ValidateAsList() const;
		bool ValidateLinks() const;

		HANDLE mHandle;
		Event* mNext;
		Event* mPrev;
		
		Event(const Event&) = delete;
		Event& operator=(const Event&) = delete;
	};

	// Note that RUNNING is an unlikely number to have in RAM by accident.
	// This helps with defensive destructor coding in the face of user error.
	enum RunState {SHUTDOWN = 0, RUNNING = 64213};

	// Internal implementation methods supporting Wait().
	Event* GetEventForWaiting();
	void RecycleEvent(Event* used_event);

	// Private critical section for access to member data.
	Mutex internalMutex;

	// Events that threads are blocked on.
	Event mWaitingList;

	// Free list for old events.
	Event mRecyclingList;
	int mRecyclingListSize = 0;

	// The number of allocated, but not yet deleted events.
	int mAllocationCounter = 0;

	RunState mRunState = RUNNING;
};

// GetEventForWaiting() provides a unique cv_event for any caller that needs to
// wait.  This means that (worst case) we may over time create as many cv_event
// objects as there are threads simultaneously using this instance's Wait()
// functionality.
XPCondVar::Event* XPCondVar::GetEventForWaiting()
{
	// We hold internal_lock, courtesy of Wait().
	Event* cvEvent;
	if(mRecyclingListSize == 0)
	{
		INTRA_DEBUG_ASSERT(mRecyclingList.IsEmpty());
		cvEvent = new Event();
		cvEvent->InitListElement();
		mAllocationCounter++;
		INTRA_DEBUG_ASSERT(cvEvent->handle());
	}
	else
	{
		cvEvent = mRecyclingList.PopFront();
		mRecyclingListSize--;
	}
	mWaitingList.PushBack(cvEvent);
	return cvEvent;
}

// RecycleEvent() takes a cv_event that was previously used for Wait()ing, and
// recycles it for use in future Wait() calls for this or other threads.
// Note that there is a tiny chance that the cv_event is still signaled when we
// obtain it, and that can cause spurious signals (if/when we re-use the
// cv_event), but such is quite rare (see FAQ-question-5).
void XPCondVar::RecycleEvent(Event* used_event)
{
	// We hold internal_lock, courtesy of Wait().
	// If the cv_event timed out, then it is necessary to remove it from
	// mWaitingList.  If it was selected by Broadcast() or Signal(), then it is
	// already gone.
	used_event->Extract();  // Possibly redundant
	mRecyclingList.PushBack(used_event);
	mRecyclingListSize++;
}

//------------------------------------------------------------------------------
// The next section provides the implementation for the private Event class.
//------------------------------------------------------------------------------

// Event provides a doubly-linked-list of events for use exclusively by the
// ConditionVariable class.

// This custom container was crafted because no simple combination of STL
// classes appeared to support the functionality required.  The specific
// unusual requirement for a linked-list-class is support for the Extract()
// method, which can remove an element from a list, potentially for insertion
// into a second list.  Most critically, the Extract() method is idempotent,
// turning the indicated element into an extracted singleton whether it was
// contained in a list or not.  This functionality allows one (or more) of
// threads to do the extraction.  The iterator that identifies this extractable
// element (in this case, a pointer to the list element) can be used after
// arbitrary manipulation of the (possibly) enclosing list container.  In
// general, STL containers do not provide iterators that can be used across
// modifications (insertions/extractions) of the enclosing containers, and
// certainly don't provide iterators that can be used if the identified
// element is *deleted* (removed) from the container.

// It is possible to use multiple redundant containers, such as an STL list,
// and an STL map, to achieve similar container semantics.  This container has
// only O(1) methods, while the corresponding (multiple) STL container approach
// would have more complex O(log(N)) methods (yeah... N isn't that large).
// Multiple containers also makes correctness more difficult to assert, as
// data is redundantly stored and maintained, which is generally evil.

XPCondVar::Event::Event():
	mHandle(null)
{
	mPrev = mNext = this;
}

XPCondVar::Event::~Event()
{
	if(mHandle == null)
	{
		// This is the list holder
		while(!IsEmpty())
		{
			Event* cvEvent = PopFront();
			INTRA_DEBUG_ASSERT(cvEvent->ValidateAsItem());
			delete cvEvent;
		}
	}
	INTRA_DEBUG_ASSERT(IsSingleton());
	if(mHandle != null)
	{
		int retVal = CloseHandle(mHandle);
		INTRA_DEBUG_ASSERT(retVal);
		(void)retVal;
	}
}

// Change a container instance permanently into an element of a list.
void XPCondVar::Event::InitListElement()
{
	INTRA_DEBUG_ASSERT(mHandle == null);
	mHandle = CreateEvent(null, false, false, null);
	INTRA_DEBUG_ASSERT(mHandle != null);
}

// Methods for use on lists.
bool XPCondVar::Event::IsEmpty() const
{
	INTRA_DEBUG_ASSERT(ValidateAsList());
	return IsSingleton();
}

void XPCondVar::Event::PushBack(Event* other)
{
	INTRA_DEBUG_ASSERT(ValidateAsList());
	INTRA_DEBUG_ASSERT(other->ValidateAsItem());
	INTRA_DEBUG_ASSERT(other->IsSingleton());
	// Prepare other for insertion.
	other->mPrev = mPrev;
	other->mNext = this;
	// Cut into list.
	mPrev->mNext = other;
	mPrev = other;
	INTRA_DEBUG_ASSERT(ValidateAsDistinct(other));
}

XPCondVar::Event* XPCondVar::Event::PopFront()
{
	INTRA_DEBUG_ASSERT(ValidateAsList());
	INTRA_DEBUG_ASSERT(!IsSingleton());
	return mNext->Extract();
}

XPCondVar::Event* XPCondVar::Event::PopBack()
{
	INTRA_DEBUG_ASSERT(ValidateAsList());
	INTRA_DEBUG_ASSERT(!IsSingleton());
	return mPrev->Extract();
}

// Methods for use on list elements.
// Accessor method.
HANDLE XPCondVar::Event::handle() const
{
	INTRA_DEBUG_ASSERT(ValidateAsItem());
	return mHandle;
}

// Pull an element from a list (if it's in one).
XPCondVar::Event* XPCondVar::Event::Extract()
{
	INTRA_DEBUG_ASSERT(ValidateAsItem());
	if(!IsSingleton())
	{
		// Stitch neighbors together.
		mNext->mPrev = mPrev;
		mPrev->mNext = mNext;
		// Make extractee into a singleton.
		mPrev = mNext = this;
	}
	INTRA_DEBUG_ASSERT(IsSingleton());
	return this;
}

// Method for use on a list element or on a list.
bool XPCondVar::Event::IsSingleton() const
{
	INTRA_DEBUG_ASSERT(ValidateLinks());
	return mNext == this;
}

// Provide pre/post conditions to validate correct manipulations.
bool XPCondVar::Event::ValidateAsDistinct(Event* other) const
{
	return ValidateLinks() && other->ValidateLinks() && (this != other);
}

bool XPCondVar::Event::ValidateAsItem() const
{
	return mHandle != null && ValidateLinks();
}

bool XPCondVar::Event::ValidateAsList() const
{
	return mHandle == null && ValidateLinks();
}

bool XPCondVar::Event::ValidateLinks() const
{
	// Make sure both of our neighbors have links that point back to us.
	// We don't do the O(n) check and traverse the whole loop, and instead only
	// do a local check to (and returning from) our immediate neighbors.
	return (mNext->mPrev == this) && (mPrev->mNext == this);
}

}}
