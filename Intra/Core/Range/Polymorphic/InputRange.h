#pragma once

#include "Utils/Unique.h"
#include "Core/Range/Span.h"
#include "Core/Range/Operations.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Concepts.h"

#include "Core/Range/Stream/InputStreamMixin.h"
#include "Utils/IInput.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#if defined(_MSC_VER) && !defined(__clang__)
#define TEMPLATE //VS doesn't compile construction typename ...<...>::template ...<...>, but it does without template
#else
#define TEMPLATE template //clang required template keyword in typename ...<...>::template ...<...>
#endif

namespace DP {

template<typename T, bool =
	CCopyAssignable<TRemoveConstRef<T>> &&
	!CAbstractClass<TRemoveConstRef<T>>
> class InputRangeInterface: public IInputEx<T> {};

template<typename T> struct InputRangeInterface<T, false>: public IInput<T>
{
	virtual size_t PopFirstN(size_t count) = 0;
};

template<typename T, typename R, typename PARENT, bool =
	CCopyAssignable<TRemoveConstRef<T>> &&
	!CAbstractClass<TRemoveConstRef<T>>
> struct InputRangeImplFiller: PARENT
{
	R OriginalRange;

	template<typename A> InputRangeImplFiller(A&& range):
		OriginalRange(Forward<A>(range)) {}

	bool Empty() const final {return OriginalRange.Empty();}
	T First() const final {return OriginalRange.First();}
	void PopFirst() final {OriginalRange.PopFirst();}

	TRemoveReference<T> Next() final
	{
		auto result = OriginalRange.First();
		OriginalRange.PopFirst();
		return result;
	}

	size_t PopFirstN(size_t count) final {return Intra::PopFirstN(OriginalRange, count);}
};

template<typename T, typename R, typename PARENT>
struct InputRangeImplFiller<T, R, PARENT, true>:
	InputRangeImplFiller<T, R, PARENT, false>
{
	typedef InputRangeImplFiller<T, R, PARENT, false> base;
	template<typename A> InputRangeImplFiller(A&& range):
		base(Forward<A>(range)) {}

	size_t ReadWrite(Span<TRemoveConstRef<T>>& dst) final
	{return Intra::ReadWrite(base::OriginalRange, dst);}
};

}

template<typename T> struct InputRange: TSelect<
	InputStreamMixin<InputRange<T>, TRemoveConst<T>>,
	EmptyType, CPod<T>>
{
protected:
	typedef DP::InputRangeInterface<T> Interface;
	template<typename R, typename PARENT> using ImplFiller = DP::InputRangeImplFiller<T, R, PARENT>;
	
	template<typename R, typename PARENT> using FullImplFiller = ImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			FullImplFiller<R, Interface>(Forward<A>(range)) {}
	};

private:
	template<typename R> using EnableCondition = Requires<
		CConvertible<TReturnValueTypeOfAs<R>, T> &&
		CAsInputRange<R> &&
		!CSameIgnoreCVRef<R, InputRange>
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef TRemoveConstRef<TRangeOfType<R&&>> Range;
		return new WrapperImpl<Range>(RangeOf(Forward<R>(range)));
	}

public:
	typedef TRemoveConstRef<T> value_type;

	constexpr forceinline InputRange(null_t=null) {}

	constexpr forceinline InputRange(InputRange&& rhs):
		mInterface(Move(rhs.mInterface)) {}

	constexpr forceinline InputRange& operator=(InputRange&& rhs)
	{
		mInterface = Move(rhs.mInterface);
		return *this;
	}

	InputRange(const InputRange& rhs) = delete;

	InputRange& operator=(const InputRange& rhs) = delete;

	template<typename R, typename = EnableCondition<R>>
	forceinline InputRange(R&& range):
		mInterface(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline InputRange& operator=(R&& range)
	{
		mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	forceinline InputRange(InitializerList<value_type> arr):
		InputRange(SpanOf(arr)) {}

	forceinline InputRange& operator=(InitializerList<value_type> arr)
	{
		operator=(SpanOf(arr));
		return *this;
	}


	INTRA_NODISCARD forceinline bool Empty() const {return mInterface == null || mInterface->Empty();}
	INTRA_NODISCARD forceinline T First() const {return mInterface->First();}
	forceinline void PopFirst() {mInterface->PopFirst();}
	INTRA_NODISCARD forceinline T Next() {return mInterface->Next();}

	forceinline size_t PopFirstN(size_t count)
	{
		if(mInterface == null) return 0;
		return mInterface->PopFirstN(count);
	}

	forceinline size_t ReadWrite(Span<value_type>& dst)
	{
		if(mInterface == null) return 0;
		return mInterface->ReadWrite(dst);
	}

	template<typename AR> Requires<
		CArrayRangeOfExactly<AR, value_type> &&
		!CConst<AR>,
	size_t> ReadWrite(AR& dst)
	{
		Span<value_type> dstArr = SpanOf(dst);
		size_t result = ReadWrite(dstArr);
		PopFirstExactly(dst, result);
		return result;
	}

	forceinline operator Interface&() {return *mInterface;}

protected:
	Unique<Interface> mInterface;
	constexpr forceinline InputRange(Interface* interfacePtr): mInterface(Move(interfacePtr)) {}
};

typedef InputRange<char> InputStream;


#undef TEMPLATE
INTRA_END
