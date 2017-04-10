#pragma once

#include "Memory/SmartRef.hh"
#include "Range/Operations.h"
#include "Algo/Op.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Concepts.h"
#include "Range/Generators/Span.h"
#include "Range/AsRange.h"
#include "Range/Stream/InputStreamMixin.h"

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

namespace DP {

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

	virtual size_t PopFirstN(size_t count) = 0;

	virtual size_t CopyAdvanceToAdvance(Span<Meta::RemoveConstRef<T>>& dst) = 0;
};

template<typename T> struct InputRangeInterface<T, false>
{
	virtual ~InputRangeInterface() {}

	virtual bool Empty() const = 0;
	virtual T First() const = 0;
	virtual void PopFirst() = 0;
	virtual T GetNext() = 0;

	virtual size_t PopFirstN(size_t count) = 0;
};

template<typename T, typename R, typename PARENT, bool Condition =
	Meta::IsCopyAssignable<Meta::RemoveConstRef<T>>::_ &&
	!Meta::IsAbstractClass<Meta::RemoveConstRef<T>>::_
> struct InputRangeImplFiller: PARENT
{
	R OriginalRange;

	template<typename A> InputRangeImplFiller(A&& range):
		OriginalRange(Meta::Forward<A>(range)) {}

	bool Empty() const final
	{return OriginalRange.Empty();}

	T First() const final
	{return OriginalRange.First();}

	void PopFirst() final
	{OriginalRange.PopFirst();}

	T GetNext() final
	{
		auto&& result = OriginalRange.First();
		OriginalRange.PopFirst();
		return result;
	}

	size_t PopFirstN(size_t count) final
	{return Range::PopFirstN(OriginalRange, count);}
};

template<typename T, typename R, typename PARENT>
struct InputRangeImplFiller<T, R, PARENT, true>:
	InputRangeImplFiller<T, R, PARENT, false>
{
	typedef InputRangeImplFiller<T, R, PARENT, false> base;
	template<typename A> InputRangeImplFiller(A&& range):
		base(Meta::Forward<A>(range)) {}

	size_t CopyAdvanceToAdvance(Span<Meta::RemoveConstRef<T>>& dst) final
	{return Algo::CopyAdvanceToAdvance(base::OriginalRange, dst);}
};

}

template<typename T> struct InputRange: Meta::SelectType<
	InputStreamMixin<InputRange<T>, Meta::RemoveConst<T>>,
	Meta::EmptyType, Meta::IsTriviallySerializable<T>::_>
{
protected:
	typedef DP::InputRangeInterface<T> Interface;
	template<typename R, typename PARENT> using ImplFiller = DP::InputRangeImplFiller<T, R, PARENT>;
	
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
	typedef Meta::RemoveConstRef<T> value_type;

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

	forceinline InputRange(InitializerList<value_type> arr):
		InputRange(AsRange(arr)) {}

	forceinline InputRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}


	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}
	forceinline T GetNext() {return mInterface->GetNext();}

	forceinline size_t PopFirstN(size_t count)
	{
		if(mInterface==null) return 0;
		return mInterface->PopFirstN(count);
	}

	forceinline size_t CopyAdvanceToAdvance(Span<value_type>& dst)
	{
		if(mInterface==null) return 0;
		return mInterface->CopyAdvanceToAdvance(dst);
	}

	template<typename AR> Meta::EnableIf<
		Range::IsArrayRangeOfExactly<AR, value_type>::_ && !Meta::IsConst<AR>::_,
	size_t> CopyAdvanceToAdvance(AR& dst)
	{
		Span<value_type> dstArr = {dst.Data(), dst.Length()};
		size_t result = CopyAdvanceToAdvance(dstArr);
		Range::PopFirstExactly(dst, result);
		return result;
	}

protected:
	Unique<Interface> mInterface;
	InputRange(Interface* interfacePtr): mInterface(Meta::Move(interfacePtr)) {}
};

typedef InputRange<char> InputStream;


#undef TEMPLATE

INTRA_WARNING_POP

}

using Range::InputRange;
using Range::InputStream;

}
