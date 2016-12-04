#pragma once

#include "Core/Core.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"


namespace Intra { namespace Range {

struct RelativeIndex
{
	enum: size_t {MaxMultiplyer = size_t(-1)};

	constexpr RelativeIndex(size_t plusValue, size_t sizeMultiplyer=0):
		multiplyer(sizeMultiplyer), plus_value(intptr(plusValue)) {}

	constexpr RelativeIndex(const RelativeIndex& rhs) = default;

	constexpr RelativeIndex operator/(intptr divisor) const
	{
		return RelativeIndex(size_t(plus_value/divisor), multiplyer/size_t(divisor));
	}

	constexpr RelativeIndex operator+(intptr value) const
	{
		return RelativeIndex(size_t(plus_value+value), multiplyer);
	}

	constexpr RelativeIndex operator+(RelativeIndex rhs) const
	{
		return RelativeIndex(size_t(plus_value+rhs.plus_value), multiplyer+rhs.multiplyer);
	}

	constexpr RelativeIndex operator-(intptr value) const
	{
		return RelativeIndex(size_t(plus_value-value), multiplyer);
	}

	constexpr RelativeIndex operator-(RelativeIndex rhs) const
	{
		return RelativeIndex(size_t(plus_value-rhs.plus_value), multiplyer-rhs.multiplyer);
	}

	friend RelativeIndex operator+(intptr value, RelativeIndex pos) {return pos+value;}
	friend RelativeIndex operator-(intptr value, RelativeIndex pos) {return RelativeIndex(0)-pos+value;}

	RelativeIndex& operator=(const RelativeIndex& rhs) = default;

	forceinline size_t GetRealIndex(size_t containerItemCount) const
	{
		if(multiplyer==0) return size_t(plus_value);
		if(multiplyer==MaxMultiplyer) return size_t(intptr(containerItemCount)+plus_value);
		return size_t(intptr(ulong64(multiplyer)*containerItemCount/MaxMultiplyer)+plus_value);
	}

private:
	size_t multiplyer;
	intptr plus_value;
};

struct RelativeIndexEnd: RelativeIndex
{
	constexpr RelativeIndexEnd():
		RelativeIndex(0, RelativeIndex::MaxMultiplyer) {}
};
constexpr const RelativeIndexEnd $;



template<typename R> struct CycleRandomResult;
template<typename RV, typename RI> struct IndexedResult;

template<typename R, typename T, class PARENT> struct RandomAccessRangeMixin: PARENT
{
private:
	forceinline const R& me() const {return *static_cast<const R*>(this);}
	forceinline R& me() {return *static_cast<R*>(this);}

public:
	forceinline R operator()(size_t first, size_t end) const
	{
		return me().opSlice(first, end);
	}

	forceinline R& Cycle() {return *this;}

	template<typename RI> forceinline IndexedResult<R, RI> Indexed(RI&& indexRange) const
	{
		return IndexedResult<R, RI>(me(), core::forward<RI>(indexRange));
	}

};

template<typename R, typename T, class PARENT> struct FiniteRandomAccessRangeMixin: PARENT
{
private:
	forceinline const R& me() const {return *static_cast<const R*>(this);}
	forceinline R& me() {return *static_cast<R*>(this);}

public:
	forceinline T& operator[](RelativeIndex pos) const
	{
		return operator[](pos.GetRealIndex(me().Length()));
	}

	forceinline R operator()(size_t firstIndex, size_t endIndex) const
	{
		return me().opSlice(firstIndex, endIndex);
	}


	forceinline R operator()(RelativeIndex firstIndex, RelativeIndex endIndex) const
	{
		return me().opSlice(
			firstIndex.GetRealIndex(me().Length()),
			endIndex.GetRealIndex(me().Length()));
	}

	forceinline R operator()(size_t firstIndex, RelativeIndex endIndex) const
	{
		return me().opSlice(firstIndex, endIndex.GetRealIndex(me().Length()));
	}

	forceinline R operator()(RelativeIndex firstIndex, size_t endIndex) const
	{
		return me().opSlice(firstIndex.GetRealIndex(me().Length()), endIndex);
	}

	forceinline size_t PopFirstN(size_t n)
	{
		const size_t l = me().Length();
		const size_t elementsToPop = n<l? n: l;
		me() = me()(elementsToPop, $);
		return elementsToPop;
	}

	forceinline void PopFirstExactly(size_t elementsToPop)
	{
		INTRA_ASSERT(elementsToPop <= me().Length());
		me() = me()(elementsToPop, $);
	}

	forceinline size_t PopLastN(size_t n)
	{
		const size_t l = me().Length();
		const size_t elementsToPop = n<l? n: l;
		me() = me()(0, l-elementsToPop);
		return elementsToPop;
	}

	forceinline void PopLastExactly(size_t elementsToPop)
	{
		INTRA_ASSERT(elementsToPop<me().Length());
		me() = me()(0, me().Length()-elementsToPop);
	}

	forceinline R Take(size_t count) const
	{
		size_t len = me().Length();
		return me().opSlice(0, len>count? count: len);
	}

	forceinline CycleRandomResult<R> Cycle() const {return CycleRandomResult<R>(me());}

	forceinline R Tail(size_t n) const
	{
		size_t len = me().Length();
		return me().opSlice(len>n? len-n: 0, len);
	}

	forceinline R& TailAdvance(size_t n)
	{
		return me() = me().Tail(n);
	}
};


}

using Range::$;

}

