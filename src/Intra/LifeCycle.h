#pragma once

#include <Intra/Core.h>

namespace Intra { INTRA_BEGIN

template<typename T> constexpr auto ConstructOneInplace = []<typename... Args>(auto* dst, Args&&... args) requires CConstructible<T, Args...>
{
	// NOTE: std::construct_at is constexpr in all cases. However, it seems impossible to use it without including standard library headers because of some compiler magic.
	if constexpr(CTriviallyDestructible<T> && CMoveAssignable<T>)
		if(IsConstantEvaluated())
		{
			*dst = T(INTRA_FWD(args)...);
			return dst;
		}
	return new(Construct, dst) T(INTRA_FWD(args)...);
};

INTRA_DEFINE_FUNCTOR(DestructOneInplace)<CDestructible T>(T* dst)
{
	// NOTE: same as the note above for std::destroy_at
	if constexpr(CTriviallyDestructible<T> && CMoveAssignable<T>)
		if(IsConstantEvaluated()) return;
	dst->~T();
};

} INTRA_END
