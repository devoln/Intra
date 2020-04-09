#pragma once

#include "Intra/Type.h"

// TODO: replace with Variant

INTRA_BEGIN
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPolymorphicCloneMethod, val.PolymorphicClone(&val, index_t()));
template<class Base, size_t SizeLimit = sizeof(Base), size_t AlignmentLimit = alignof(Base)> class Polymorphic
{
	static_assert(CHasVirtualDestructor<Base> && CHasPolymorphicCloneMethod<Base>);
	static_assert(SizeLimit >= sizeof(Base));
	static_assert(AlignmentLimit >= alignof(Base));
	alignas(AlignmentLimit) union
	{
		char dummy[SizeLimit];
		Base mVal;
	};
public:
	INTRA_FORCEINLINE Polymorphic(const Base& rhs) {rhs.mVal.PolymorphicClone(&mVal, SizeLimit, false);}
	INTRA_FORCEINLINE Polymorphic(Base&& rhs) {rhs.mVal.PolymorphicClone(&mVal, SizeLimit, true);}
	template<class Derived, typename... Args>
	explicit INTRA_FORCEINLINE Polymorphic(TConstructT<Derived>, Args&&... args)
	{
		static_assert(CSame<Base, Derived> ||
			CDerived<Derived, Base> && CHasVirtualDestructor<Base> && CHasPolymorphicCloneMethod<Base>);
		static_assert(sizeof(Derived) <= SizeLimit, "Type Derived is too big for this Polymorphic object!");
		static_assert(alignof(Derived) <= AlignmentLimit, "Type Derived alignment requirement exceeds limit!");
		new(&mVal) Derived(Forward<Args>(args)...);
	}

	constexpr operator Base&() {return mVal;}
	constexpr operator const Base&() const {return mVal;}
};
INTRA_END
