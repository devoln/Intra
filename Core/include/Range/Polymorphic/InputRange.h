#pragma once

#include "Memory/SmartRef.hh"
#include "Range/Operations.h"
#include "Algo/Op.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Concepts.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/AsRange.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#if defined(_MSC_VER) && !defined(__clang__)
#define TEMPLATE //VS не хочет компилировать конструкцию typename ...<...>::template ...<...>, но без template компилирует
#else
#define TEMPLATE template //clang требует слово template в typename ...<...>::template ...<...>
#endif

namespace D {

template<typename T, bool Condition =
	Meta::IsCopyAssignable<Meta::RemoveConstRef<T>>::_ &&
	!Meta::IsAbstractClass<Meta::RemoveConstRef<T>>::_
> struct InputRangeInterface
{
	virtual ~InputRangeInterface() {}

	virtual bool Empty() const = 0;
	virtual T First() const = 0;
	virtual void PopFirst() = 0;
	virtual T GetNext() = 0;

	virtual void PopFirstN(size_t count) = 0;
	virtual void PopFirstExactly(size_t count) = 0;

	virtual size_t CopyAdvanceToAdvance(ArrayRange<Meta::RemoveConstRef<T>>& dst) = 0;
};

template<typename T> struct InputRangeInterface<T, false>
{
	virtual ~InputRangeInterface() {}

	virtual bool Empty() const = 0;
	virtual T First() const = 0;
	virtual void PopFirst() = 0;
	virtual T GetNext() = 0;

	virtual void PopFirstN(size_t count) = 0;
	virtual void PopFirstExactly(size_t count) = 0;
};

template<typename T, typename R, typename PARENT, bool Condition =
	Meta::IsCopyAssignable<Meta::RemoveConstRef<T>>::_ &&
	!Meta::IsAbstractClass<Meta::RemoveConstRef<T>>::_
> struct InputRangeImplFiller: PARENT
{
	R OriginalRange;

	template<typename A> InputRangeImplFiller(A&& range):
		OriginalRange(Meta::Forward<A>(range)) {}

	bool Empty() const override
	{return OriginalRange.Empty();}

	T First() const override
	{return OriginalRange.First();}

	void PopFirst() override
	{OriginalRange.PopFirst();}

	T GetNext() override
	{
		auto&& result = OriginalRange.First();
		OriginalRange.PopFirst();
		return result;
	}

	void PopFirstN(size_t count) override
	{Range::PopFirstN(OriginalRange, count);}
	void PopFirstExactly(size_t count) override
	{Range::PopFirstExactly(OriginalRange, count);}
};

template<typename T, typename R, typename PARENT>
struct InputRangeImplFiller<T, R, PARENT, true>:
	InputRangeImplFiller<T, R, PARENT, false>
{
	typedef InputRangeImplFiller<T, R, PARENT, false> base;
	template<typename A> InputRangeImplFiller(A&& range):
		base(Meta::Forward<A>(range)) {}

	size_t CopyAdvanceToAdvance(ArrayRange<Meta::RemoveConstRef<T>>& dst) override
	{return Algo::CopyAdvanceToAdvance(base::OriginalRange, dst);}
};

}

template<typename T> struct InputRange
{
protected:
	typedef Meta::RemoveReference<T> value_type;
	typedef D::InputRangeInterface<T> Interface;
	template<typename R, typename PARENT> using ImplFiller = D::InputRangeImplFiller<T, R, PARENT>;
	
	template<typename R, typename PARENT> using FullImplFiller = ImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			FullImplFiller<R, Interface>(Meta::Forward<A>(range)) {}
	};

private:
	template<typename R> using EnableCondition = Meta::EnableIf<
		Meta::IsConvertible<ReturnValueTypeOfAs<R>, T>::_ &&
		IsAsInputRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, InputRange>::_
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<AsRangeResult<R&&>> Range;
		return new WrapperImpl<Range>(AsRange(Meta::Forward<R>(range)));
	}

public:
	forceinline InputRange(null_t=null): mInterface(null) {}

	forceinline InputRange(InputRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

	forceinline InputRange& operator=(InputRange&& rhs)
	{
		mInterface = Meta::Move(rhs.mInterface);
		return *this;
	}

	InputRange(const InputRange& rhs) = delete;

	InputRange& operator=(const InputRange& rhs) = delete;

	template<typename R, typename = EnableCondition<R>>
	forceinline InputRange(R&& range):
		mInterface(wrap(Range::Forward<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline InputRange& operator=(R&& range)
	{
		mInterface = wrap(Range::Forward<R>(range));
		return *this;
	}

	forceinline InputRange(InitializerList<Meta::RemoveConst<value_type>> arr):
		InputRange(AsRange(arr)) {}

	forceinline InputRange& operator=(InitializerList<Meta::RemoveConst<value_type>> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}


	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}
	forceinline T GetNext() {return mInterface->GetNext();}

	forceinline void PopFirstN(size_t count) {mInterface->PopFirstN(count);}
	forceinline void PopFirstExactly(size_t count) {mInterface->PopFirstExactly(count);}

	forceinline size_t CopyAdvanceToAdvance(ArrayRange<Meta::RemoveConst<value_type>>& dst)
	{return mInterface->CopyAdvanceToAdvance(dst);}

protected:
	Memory::UniqueRef<Interface> mInterface;
	InputRange(Interface* interfacePtr): mInterface(interfacePtr) {}
};


#undef TEMPLATE

INTRA_WARNING_POP

}

using Range::InputRange;

}
