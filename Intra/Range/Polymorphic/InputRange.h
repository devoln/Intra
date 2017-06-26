#pragma once

#include "Utils/Unique.h"
#include "Utils/Span.h"
#include "Range/Operations.h"
#include "Utils/Op.h"
#include "Range/Mutation/Copy.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Range/Stream/InputStreamMixin.h"
#include "Concepts/IInput.h"

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

template<typename T, bool =
	Meta::IsCopyAssignable<Meta::RemoveConstRef<T>>::_ &&
	!Meta::IsAbstractClass<Meta::RemoveConstRef<T>>::_
> class InputRangeInterface: public IInputEx<T> {};

template<typename T> struct InputRangeInterface<T, false>: public IInput<T>
{
	virtual size_t PopFirstN(size_t count) = 0;
};

template<typename T, typename R, typename PARENT, bool =
	Meta::IsCopyAssignable<Meta::RemoveConstRef<T>>::_ &&
	!Meta::IsAbstractClass<Meta::RemoveConstRef<T>>::_
> struct InputRangeImplFiller: PARENT
{
	R OriginalRange;

	template<typename A> InputRangeImplFiller(A&& range):
		OriginalRange(Cpp::Forward<A>(range)) {}

	bool Empty() const final {return OriginalRange.Empty();}
	T First() const final {return OriginalRange.First();}
	void PopFirst() final {OriginalRange.PopFirst();}

	Meta::RemoveReference<T> Next() final
	{
		auto result = OriginalRange.First();
		OriginalRange.PopFirst();
		return result;
	}

	size_t PopFirstN(size_t count) final {return Range::PopFirstN(OriginalRange, count);}
};

template<typename T, typename R, typename PARENT>
struct InputRangeImplFiller<T, R, PARENT, true>:
	InputRangeImplFiller<T, R, PARENT, false>
{
	typedef InputRangeImplFiller<T, R, PARENT, false> base;
	template<typename A> InputRangeImplFiller(A&& range):
		base(Cpp::Forward<A>(range)) {}

	size_t ReadToAdvance(Span<Meta::RemoveConstRef<T>>& dst) final
	{return Range::ReadToAdvance(base::OriginalRange, dst);}
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
			FullImplFiller<R, Interface>(Cpp::Forward<A>(range)) {}
	};

private:
	template<typename R> using EnableCondition = Meta::EnableIf<
		Meta::IsConvertible<Concepts::ReturnValueTypeOfAs<R>, T>::_ &&
		Concepts::IsAsInputRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, InputRange>::_
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<Concepts::RangeOfType<R&&>> Range;
		return new WrapperImpl<Range>(RangeOf(Cpp::Forward<R>(range)));
	}

public:
	typedef Meta::RemoveConstRef<T> value_type;

	forceinline InputRange(null_t=null): mInterface(null) {}

	forceinline InputRange(InputRange&& rhs):
		mInterface(Cpp::Move(rhs.mInterface)) {}

	forceinline InputRange& operator=(InputRange&& rhs)
	{
		mInterface = Cpp::Move(rhs.mInterface);
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
		InputRange(SpanOf(arr)) {}

	forceinline InputRange& operator=(InitializerList<value_type> arr)
	{
		operator=(SpanOf(arr));
		return *this;
	}


	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}
	forceinline T Next() {return mInterface->Next();}

	forceinline size_t PopFirstN(size_t count)
	{
		if(mInterface==null) return 0;
		return mInterface->PopFirstN(count);
	}

	forceinline size_t ReadToAdvance(Span<value_type>& dst)
	{
		if(mInterface==null) return 0;
		return mInterface->ReadToAdvance(dst);
	}

	template<typename AR> Meta::EnableIf<
		Concepts::IsArrayRangeOfExactly<AR, value_type>::_ &&
		!Meta::IsConst<AR>::_,
	size_t> ReadToAdvance(AR& dst)
	{
		Span<value_type> dstArr = SpanOf(dst);
		size_t result = ReadToAdvance(dstArr);
		PopFirstExactly(dst, result);
		return result;
	}

	forceinline operator Interface() {return *mInterface;}

protected:
	Unique<Interface> mInterface;
	InputRange(Interface* interfacePtr): mInterface(Cpp::Move(interfacePtr)) {}
};

typedef InputRange<char> InputStream;


#undef TEMPLATE

INTRA_WARNING_POP

}

using Range::InputRange;
using Range::InputStream;

}
