#pragma once

#include "IntraX/Utils/Unique.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Concepts.h"

#include "Intra/Range/Stream/InputStreamMixin.h"
#include "Intra/Range/Polymorphic/TypeErasure.h"

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
//INTRA_IGNORE_WARN_SIGN_CONVERSION

template<typename T> struct InputRange
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
		!CSameUnqualRef<R, InputRange>
	>;

	template<typename R> static Interface* wrap(R&& range)
	{
		using Range = TRemoveConstRef<TRangeOfRef<R&&>>;
		return new WrapperImpl<Range>(RangeOf(Forward<R>(range)));
	}

public:
	typedef TRemoveConstRef<T> value_type;

	constexpr InputRange(decltype(nullptr)=nullptr) {}

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


	[[nodiscard]] bool Empty() const {return mInterface == nullptr || mInterface->Empty();}
	[[nodiscard]] T First() const {return mInterface->First();}
	void PopFirst() {mInterface->PopFirst();}
	[[nodiscard]] T Next() {return mInterface->Next();}

	auto PopFirstCount(index_t count)
	{
		if(mInterface == nullptr) return 0;
		return mInterface->PopFirstCount(count);
	}

	index_t ReadWrite(Span<value_type>& dst)
	{
		if(mInterface == nullptr) return 0;
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

} INTRA_END
