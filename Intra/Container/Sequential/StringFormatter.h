#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Concepts/Container.h"

#include "Utils/Span.h"
#include "Utils/StringView.h"

#include "Range/Operations.h"
#include "Range/Output/Inserter.h"
#include "Range/Stream/ToStringArithmetic.h"
#include "Range/Stream/ToString.h"
#include "Range/Stream/MaxLengthOfToString.h"
#include "Range/Search/Subrange.h"

#include "Container/Operations.hh"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION

namespace Intra { namespace Container {

template<typename S> class StringFormatter
{
	static_assert(Concepts::Has_data<S>::_, "S must contain data method!");
	static_assert(Concepts::Has_size<S>::_, "S must contain size method!");
	static_assert(Concepts::IsSequentialContainer<S>::_, "S must be a sequential container!");
	typedef Concepts::ElementTypeOfArray<S> Char;
public:
	operator S()
	{
		INTRA_DEBUG_ASSERT(mFormatRest.Empty());
		DiscardBuffer();
		return Cpp::Move(mResult);
	}
	forceinline S operator*() {return operator S();}

private:
	void WriteNextPart()
	{
		RequireSpace(mFormatRest.Length());
		size_t partLen = CountUntil(mFormatRest, "<^>");
		Range::ReadWrite(mFormatRest, partLen, mBufferRest);
		mFormatRest.PopFirstN(3);
	}

	void RequireSpace(size_t newChars)
	{
		if(mBufferRest.Length()>=newChars) return; //Места достаточно, увеличивать буфер не надо
		const size_t currentLength = CurrentLength();
		size_t newSize = currentLength + Math::Max(currentLength, newChars);
		if(newSize<16) newSize=16;
		Concepts::SetCountTryNotInit(mResult, newSize);
		mBufferRest = Range::Drop(mResult, currentLength);
	}

	forceinline size_t CurrentLength() const
	{return mResult.size()-mBufferRest.Length();}

	void DiscardBuffer()
	{
		Concepts::SetCountTryNotInit(mResult, CurrentLength());
		mBufferRest = null;
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
		mFormatRest(rhs.mFormatRest), mResult(Cpp::Move(rhs.mResult)),
		mBufferRest(rhs.mBufferRest) {rhs.mBufferRest = null;}

	~StringFormatter() {INTRA_DEBUG_ASSERT(mFormatRest.Empty());}

	template<typename T, typename... Args> Meta::EnableIf<
		Meta::IsCopyConstructible<Meta::RemoveConstRef<T>>::_ &&
		!(!Concepts::IsInputRange<T>::_ && Concepts::HasRangeOf<T>::_),
	StringFormatter&> operator()(T&& value, Args&&... args)
	{
		const size_t maxLen = Range::MaxLengthOfToString(value, args...);
		RequireSpace(maxLen);
		ToString(mBufferRest, Cpp::Forward<T>(value), Cpp::Forward<Args>(args)...);
		if(mFormatRest != null) WriteNextPart();
		return *this;
	}

	template<typename T, typename... Args> Meta::EnableIf<
		!Meta::IsCopyConstructible<Meta::RemoveConstRef<T>>::_ &&
		!Meta::IsConst<T>::_ &&
		!(!Concepts::IsInputRange<T>::_ &&
			Concepts::HasRangeOf<T>::_),
	StringFormatter&> operator()(T&& value, Args&&... args)
	{
		DiscardBuffer();
		ToString(Range::LastAppender(mResult), Cpp::Forward<T>(value), Cpp::Forward<Args>(args)...);
		if(mFormatRest!=null) WriteNextPart();
		return *this;
	}

	template<typename T, typename... Args> forceinline Meta::EnableIf<
		!Concepts::IsInputRange<T>::_ &&
		Concepts::HasRangeOf<T>::_,
	StringFormatter&> operator()(T&& value, Args&&... args)
	{return operator()(Range::Forward<T>(value), Cpp::Forward<Args>(args)...);}

	template<size_t N> forceinline StringFormatter& operator()(const Char(&arr)[N])
	{return operator()(GenericStringView<const Char>(arr));}

	forceinline StringFormatter& operator()(const Char* cstr)
	{return operator()(GenericStringView<const Char>(cstr));}

	template<typename Arg0> forceinline StringFormatter& Arg(Arg0&& arg0)
	{return operator()(Cpp::Forward<Arg0>(arg0));}

	template<typename Arg0, typename Arg1, typename... Args> StringFormatter& Arg(Arg0&& arg0, Arg1&& arg1, Args&&... args)
	{
		operator()(Cpp::Forward<Arg0>(arg0));
		return Arg(Cpp::Forward<Arg1>(arg1), Cpp::Forward<Args>(args)...);
	}
};

}}

INTRA_WARNING_POP
