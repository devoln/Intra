#pragma once

#include "IntraX/Utils/Unique.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Concepts.h"

#include "Intra/Range/Stream/InputStreamMixin.h"
#include "Intra/Range/Polymorphic/IInput.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
//INTRA_IGNORE_WARN_SIGN_CONVERSION

namespace z_D {

template<typename T, bool =
	CCopyAssignable<TRemoveConstRef<T>> &&
	!CAbstractClass<TRemoveConstRef<T>>
> class InputRangeInterface: public IInputEx<T> {};

template<typename T> struct InputRangeInterface<T, false>: public IInput<T>
{
	virtual index_t PopFirstCount(ClampedSize count) = 0;
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

	index_t PopFirstCount(ClampedSize count) final {return Intra::PopFirstCount(OriginalRange, count);}
};

template<typename T, typename R, typename PARENT>
struct InputRangeImplFiller<T, R, PARENT, true>:
	InputRangeImplFiller<T, R, PARENT, false>
{
	typedef InputRangeImplFiller<T, R, PARENT, false> base;
	template<typename A> InputRangeImplFiller(A&& range):
		base(Forward<A>(range)) {}

	index_t ReadWrite(Span<TRemoveConstRef<T>>& dst) final
	{return Intra::ReadWrite(base::OriginalRange, dst);}
};

}

template<typename T> struct InputRange: TSelect<
	InputStreamMixin<InputRange<T>, TRemoveConst<T>>,
	EmptyType, CChar<T>>
{
protected:
	typedef z_D::InputRangeInterface<T> Interface;
	template<typename R, typename PARENT> using ImplFiller = z_D::InputRangeImplFiller<T, R, PARENT>;
	
	template<typename R, typename PARENT> using FullImplFiller = ImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			FullImplFiller<R, Interface>(Forward<A>(range)) {}
	};

private:
	template<typename R> using EnableCondition = Requires<
		CConvertibleTo<TListValueRef<R>, T> &&
		CList<R> &&
		!CSameIgnoreCVRef<R, InputRange>
	>;

	template<typename R> static Interface* wrap(R&& range)
	{
		using Range = TRemoveConstRef<TRangeOfRef<R&&>>;
		return new WrapperImpl<Range>(RangeOf(Forward<R>(range)));
	}

public:
	typedef TRemoveConstRef<T> value_type;

	constexpr InputRange(decltype(null)=null) {}

	constexpr InputRange(InputRange&& rhs):
		mInterface(Move(rhs.mInterface)) {}

	constexpr InputRange& operator=(InputRange&& rhs)
	{
		mInterface = Move(rhs.mInterface);
		return *this;
	}

	InputRange(const InputRange& rhs) = delete;

	InputRange& operator=(const InputRange& rhs) = delete;

	template<typename R, typename = EnableCondition<R>>
	InputRange(R&& range):
		mInterface(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	InputRange& operator=(R&& range)
	{
		mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	InputRange(InitializerList<value_type> arr):
		InputRange(SpanOf(arr)) {}

	InputRange& operator=(InitializerList<value_type> arr)
	{
		operator=(SpanOf(arr));
		return *this;
	}


	[[nodiscard]] bool Empty() const {return mInterface == null || mInterface->Empty();}
	[[nodiscard]] T First() const {return mInterface->First();}
	void PopFirst() {mInterface->PopFirst();}
	[[nodiscard]] T Next() {return mInterface->Next();}

	auto PopFirstCount(index_t count)
	{
		if(mInterface == null) return 0;
		return mInterface->PopFirstCount(count);
	}

	index_t ReadWrite(Span<value_type>& dst)
	{
		if(mInterface == null) return 0;
		return mInterface->ReadWrite(dst);
	}

	template<typename AR> Requires<
		CArrayRangeOfExactly<AR, value_type> &&
		!CConst<AR>,
	index_t> ReadWrite(AR& dst)
	{
		Span<value_type> dstArr = SpanOf(dst);
		size_t result = ReadWrite(dstArr);
		PopFirstExactly(dst, result);
		return result;
	}

	operator Interface&() {return *mInterface;}

protected:
	Unique<Interface> mInterface;
	constexpr InputRange(Interface* interfacePtr): mInterface(Move(interfacePtr)) {}
};

using InputStream = InputRange<char>;

INTRA_END
