#pragma once

#include "Core/Type.h"

INTRA_CORE_BEGIN
template<class Base, size_t SizeLimit = sizeof(Base), size_t AlignmentLimit = alignof(Base)> class Polymorphic
{
	static_assert(CHasVirtualDestructor<Base> && CHasPolymorphicCloneMethod<T>, "Bad Base type for Polymorphic<Base>!");
	static_assert(SizeLimit >= sizeof(Base), "SizeLimit is too low! It must be higher or equal to sizeof(Base).");
	static_assert(AlignmentLimit >= alignof(Base), "AlignmentLimit is too low! It must be higher or equal to alignof(Base).");
	alignas(AlignmentLimit) union
	{
		char dummy[SizeLimit];
		Base mVal;
	};
public:
	forceinline Polymorphic(const Base& rhs) {rhs.mVal.PolymorphicClone(&mVal, SizeLimit, false);}
	forceinline Polymorphic(Base&& rhs) {rhs.mVal.PolymorphicClone(&mVal, SizeLimit, true);}
	template<class Derived, typename... Args> explicit forceinline Polymorphic(TConstructT<Derived>, Args&& ... args)
	{
		static_assert(CSame<Base, Derived> || CDerived<U, T> && CHasVirtualDestructor<T> && CHasPolymorphicCloneMethod<T>, "Bad type Derived passed to Polymorphic<T> constructor.");
		static_assert(sizeof(Derived) <= SizeLimit || sizeof(Derived) <= sizeof(Base), "Type Derived is too big to be stored in this Polymorphic object!");
		static_assert(alignof(Derived) <= AlignmentLimit || alignof(Derived) <= alignof(Base), "Type Derived alignment requirement exceeds AlignmentLimit!");
		new(&mVal) Derived(Forward<Args>(args)...);
	}

	INTRA_CONSTEXPR2 forceinline operator Base&() {return mVal;}
	constexpr forceinline operator const Base&() const {return mVal;}
};
INTRA_CORE_END
