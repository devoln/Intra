#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Operations.h"
#include "Algo/String/ToStringArithmetic.h"
#include "Algo/String/ToString.h"
#include "Algo/String/MaxLengthOfToString.h"
#include "Range/Generators/StringView.h"
#include "Container/Operations.hh"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION

namespace Intra { namespace Container {

template<typename S> class StringFormatter
{
	static_assert(Has_data<S>::_, "S must contain data method!");
	static_assert(Has_size<S>::_, "S must contain size method!");
	static_assert(IsSequentialContainer<S>::_, "S must be a sequential container!");
	typedef ValueTypeOf<S> Char;
public:
	operator S()
	{
		INTRA_ASSERT(mFormatRest.Empty());
		Container::SetCountTryNotInit(mResult, mResult.size()-mBufferRest.Length());
		mBufferRest = null;
		return Meta::Move(mResult);
	}
	forceinline S operator*() {return operator S();}

private:
	void WriteNextPart()
	{
		RequireSpace(mFormatRest.Length());
		size_t partLen = Algo::CountUntil(mFormatRest, "<^>");
		Algo::CopyAdvanceToAdvance(mFormatRest, partLen, mBufferRest);
		mFormatRest.PopFirstN(3);
	}

	void RequireSpace(size_t newChars)
	{
		if(mBufferRest.Length()>=newChars) return; //Места достаточно, увеличивать буфер не надо
		size_t currentLength = mResult.size()-mBufferRest.Length();
		size_t newSize = currentLength + Math::Max(currentLength, newChars);
		if(newSize<16) newSize=16;
		Container::SetCountTryNotInit(mResult, newSize);
		mBufferRest = Range::Drop(mResult, currentLength);
	}

	GenericStringView<const Char> mFormatRest;
	S mResult;
	GenericStringView<Char> mBufferRest;

	StringFormatter(const StringFormatter&) = delete;
	StringFormatter& operator=(const StringFormatter&) = delete;

public:
	StringFormatter(GenericStringView<const Char> formatStr):
		mFormatRest(formatStr),
		mResult(), mBufferRest(null) {if(mFormatRest!=null) WriteNextPart();}

	StringFormatter(StringFormatter&& rhs):
		mFormatRest(rhs.mFormatRest), mResult(Meta::Move(rhs.mResult)),
		mBufferRest(rhs.mBufferRest) {rhs.mBufferRest = null;}

	~StringFormatter() {INTRA_ASSERT(mFormatRest.Empty());}

	template<typename T, typename... Args> StringFormatter& operator()(const T& value, Args&&... args)
	{
		const size_t maxLen = Algo::MaxLengthOfToString(value, args...);
		RequireSpace(maxLen);
		//Range::CountRange<char> realLenCounter;
		//Algo::ToString(realLenCounter, value, args...);
		//INTRA_ASSERT(maxLen>=realLenCounter.Counter);
		Algo::ToString(mBufferRest, value, Meta::Forward<Args>(args)...);
		if(mFormatRest!=null) WriteNextPart();
		return *this;
	}

	template<typename T, typename... Args> Meta::EnableIf<
		!Range::IsInputRange<T>::_ && Range::HasAsRange<T>::_,
	StringFormatter&> operator()(T&& value, Args&&... args)
	{return operator()(Range::AsRange(value), Meta::Forward<Args>(args)...);}

	forceinline StringFormatter& operator()(const Char* arr)
	{return operator()(GenericStringView<const Char>(arr));}
};

}}

INTRA_WARNING_POP
