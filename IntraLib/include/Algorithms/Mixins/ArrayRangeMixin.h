#pragma once

#include "Core/Core.h"
#include "Meta/Type.h"
#include "Algorithms/RangeConcept.h"


namespace Intra { namespace Range {


template<typename R, typename T, class PARENT> struct ArrayRangeMixin: PARENT
{
private:
	forceinline const R& me() const {return *static_cast<const R*>(this);}
	forceinline R& me() {return *static_cast<R*>(this);}
public:

	template<typename RW> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<RW, T>::_,
	bool> StartsWith(const RW& what) const
	{
		if(me().Length()<what.Length()) return false;
		return core::memcmp(me().Data(), what.Data(), what.Length()*sizeof(T))==0;
	}

	template<size_t N> forceinline bool StartsWith(const T(&rhs)[N]) const {return me().StartsWith(AsRange(rhs));}

	

	template<typename RW> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<RW, T>::_,
	bool> EndsWith(const RW& what) const
	{
		if(me().Length()<what.Length()) return false;
		return core::memcmp(me().Data()+me().Length()-what.Length(), what.Data(), what.Length()*sizeof(T))==0;
	}

	template<size_t N> forceinline bool EndsWith(const T(&rhs)[N]) {return me().EndsWith(AsRange(rhs));}


	template<typename OR> forceinline Meta::EnableIf<
		!IsArrayRangeOfExactly<OR, T>::_ && IsOutputRange<OR>::_ && !Meta::IsConst<OR>::_
	> CopyAdvanceToAdvance(OR&& dst) {PARENT::CopyAdvanceToAdvance(dst);}

	template<typename OR> forceinline Meta::EnableIf<
		!IsArrayRangeOfExactly<OR, T>::_ && IsOutputRange<OR>::_
	> CopyAdvanceTo(const OR& dst) {PARENT::CopyAdvanceTo(dst);}

	template<typename OR> forceinline Meta::EnableIf<
		!IsArrayRangeOfExactly<OR, T>::_ && IsOutputRange<OR>::_
	> CopyTo(const OR& dst) const {PARENT::CopyTo(dst);}



	template<typename OR, typename P> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_ && !Meta::IsConst<OR>::_
	> CopyAdvanceToAdvance(OR&& dst, P pred) {PARENT::CopyAdvanceToAdvance(dst, pred);}

	template<typename OR, typename P> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_
	> CopyAdvanceTo(const OR& dst, P pred) {PARENT::CopyAdvanceTo(dst, pred);}

	template<typename OR, typename P> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_ && !Meta::IsConst<OR>::_
	> CopyToAdvance(OR&& dst, P pred) const {PARENT::CopyToAdvance(dst, pred);}

	template<typename OR, typename P> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_
	> CopyTo(const OR& dst, P pred) const {PARENT::CopyTo(dst, pred);}



	template<typename DstRange> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<DstRange, T>::_
	> CopyAdvanceTo(const DstRange& dst)
	{
		me().CopyTo(dst);
		me().PopFirstExactly(me().Length());
	}

	template<typename DstRange> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<DstRange, T>::_ && !Meta::IsConst<DstRange>::_
	> CopyAdvanceToAdvance(DstRange&& dst)
	{
		me().CopyToAdvance(dst);
		me().PopFirstExactly(me().Length());
	}

	template<typename DstRange> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<DstRange, T>::_
	> CopyTo(const DstRange& dst) const
	{
		INTRA_ASSERT(dst.Length()>=me().Length());
		core::memcpy(dst.Data(), me().Data(), me().Length());
	}

	template<typename DstRange> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<DstRange, T>::_ && !Meta::IsConst<DstRange>::_
	> CopyToAdvance(DstRange&& dst) const
	{
		me().CopyTo(dst);
		dst.PopFirstExactly(me().Length());
	}

};


}}

