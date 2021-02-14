#pragma once

#include "Intra/Range.h"

namespace Intra { INTRA_BEGIN
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
	[[nodiscard]] INTRA_FORCEINLINE T* NextListNode() const {return mNext;}
	[[nodiscard]] INTRA_FORCEINLINE static auto Instances() {return RangeOf(*lastInited);}
};
template<typename T> T* GloballyRegistered<T>::lastInited;
} INTRA_END
