#pragma once

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

#include "Utils/Optional.h"

#include "Funal/Method.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R, typename F> struct RMap:
	Meta::CopyableIf<Meta::IsCopyConstructible<R>::_ && Meta::IsCopyConstructible<F>::_>
{
private:
	typedef Meta::ResultOf<F, Concepts::ReturnValueTypeOf<R>> ReturnValueType;
public:
	enum: bool
	{
		RangeIsFinite = Concepts::IsFiniteRange<R>::_,
		RangeIsInfinite = Concepts::IsInfiniteRange<R>::_
	};

	forceinline RMap(null_t=null):
		OriginalRange(null), Function() {}

	forceinline RMap(R&& range, F func):
		OriginalRange(Cpp::Move(range)), Function(func) {}

	forceinline RMap(const R& range, F func):
		OriginalRange(range), Function(func) {}


	forceinline ReturnValueType First() const
	{return Function()(OriginalRange.First());}

	forceinline void PopFirst()
	{OriginalRange.PopFirst();}

	forceinline bool Empty() const
	{return OriginalRange.Empty() || Function==null;}

	forceinline ReturnValueType Last() const
	{return Function()(OriginalRange.Last());}

	forceinline void PopLast() {OriginalRange.PopLast();}

	forceinline ReturnValueType operator[](size_t index) const
	{return Function()(OriginalRange[index]);}

	template<typename U = R> forceinline Meta::EnableIf<
		Concepts::HasSlicing<U>::_,
	RMap<Concepts::SliceTypeOf<U>, F>> operator()(size_t start, size_t end) const
	{return {OriginalRange(start, end), Function()};}

	template<typename U = R> forceinline Meta::EnableIf<
		Concepts::HasLength<U>::_,
	size_t> Length() const
	{return OriginalRange.Length();}

	forceinline bool operator==(const RMap& rhs) const
	{
		return Function == rhs.Function &&
			OriginalRange == rhs.OriginalRange;
	}

	R OriginalRange;
	Utils::Optional<F> Function;
};


template<typename R, typename F,
	typename AsR = Concepts::RangeOfType<R>,
	typename T = Concepts::ReturnValueTypeOf<AsR>
> forceinline Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_ &&
	Meta::IsCallable<F, T>::_,
RMap<Meta::RemoveConstRef<AsR>, F>> Map(R&& range, F func)
{return {Range::Forward<R>(range), Cpp::Move(func)};}

template<typename R, typename RET,
	typename AsR = Concepts::RangeOfType<R>,
	typename T = Concepts::ValueTypeOf<AsR>
> forceinline Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_,
RMap<Meta::RemoveConstRef<AsR>, Funal::ConstMethodWrapper<T, RET>>> Map(R&& range, RET(T::*func)() const)
{return {Range::Forward<R>(range), Funal::Method(func)};}

INTRA_WARNING_POP

}}
