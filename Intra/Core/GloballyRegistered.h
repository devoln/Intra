#pragma once

#include "Core/Range/ListRange.h"

INTRA_BEGIN
template<typename T> class GloballyRegistered
{
	T* mNext;
	static T* lastInited;

	GloballyRegistered(const GloballyRegistered&) = delete;
	GloballyRegistered(GloballyRegistered&&) = delete;
	GloballyRegistered& operator=(const GloballyRegistered&) = delete;
	GloballyRegistered& operator=(GloballyRegistered&&) = delete;
protected:
	GloballyRegistered(): mNext(lastInited) {lastInited = static_cast<T*>(this);}
public:
	INTRA_NODISCARD forceinline T* NextListNode() const {return mNext;}
	INTRA_NODISCARD forceinline static auto Instances() {return RangeOf(*lastInited);}
};
template<typename T> T* GloballyRegistered<T>::lastInited;
INTRA_END
