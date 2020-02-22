#pragma once

#include "Core/CContainer.h"

#include "Core/Range/Concepts.h"
#include "Core/Range/Span.h"
#include "Core/Range/StringView.h"
#include "Core/Range/Operations.h"
#include "Core/Range/Inserter.h"
#include "Core/Range/Stream/ToStringArithmetic.h"
#include "Core/Range/Stream/ToString.h"
#include "Core/Range/Stream/MaxLengthOfToString.h"
#include "Core/Range/Search/Subrange.h"

#include "Container/Operations.hh"

INTRA_BEGIN
INTRA_WARNING_DISABLE_SIGN_CONVERSION
//TODO: replace this with complete formatting solution with positional and named argument support and argument formatting specification in a format string.

template<typename S> class StringFormatter
{
	static_assert(CHas_data<S>, "S must contain data method!");
	static_assert(CHas_size<S>, "S must contain size method!");
	static_assert(CSequentialContainer<S>, "S must be a sequential container!");
	typedef TArrayElement<S> Char;
public:
	operator S()
	{
		INTRA_PRECONDITION(mFormatRest.Empty());
		DiscardBuffer();
		return Move(mResult);
	}
	forceinline S operator*() {return operator S();}

private:
	void WriteNextPart()
	{
		RequireSpace(mFormatRest.Length());
		const index_t partLen = CountUntil(mFormatRest, "<^>");
		const index_t charsWritten = WriteTo(mFormatRest.Take(partLen), mBufferRest);
		mFormatRest.PopFirstN(charsWritten+3);
	}

	void RequireSpace(index_t newChars)
	{
		if(mBufferRest.Length() >= newChars) return; //No need to reallocate buffer
		const size_t currentLength = CurrentLength();
		size_t newSize = currentLength + Max(currentLength, newChars);
		if(newSize < 16) newSize = 16;
		SetCountTryNotInit(mResult, newSize);
		mBufferRest = Drop(mResult, currentLength);
	}

	forceinline index_t CurrentLength() const
	{return mResult.size()-mBufferRest.Length();}

	void DiscardBuffer()
	{
		SetCountTryNotInit(mResult, CurrentLength());
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
		mResult(), mBufferRest(null)
	{
		if(mFormatRest != null)
			WriteNextPart();
	}

	StringFormatter(StringFormatter&& rhs):
		mFormatRest(rhs.mFormatRest), mResult(Move(rhs.mResult)),
		mBufferRest(rhs.mBufferRest) {rhs.mBufferRest = null;}

	~StringFormatter() {INTRA_PRECONDITION(mFormatRest.Empty());}

	template<typename T, typename... Args> Requires<
		CCopyConstructible<TRemoveConstRef<T>> &&
		!(!CInputRange<T> && CHasRangeOf<T>),
	StringFormatter&> operator()(T&& value, Args&&... args)
	{
		const size_t maxLen = MaxLengthOfToString(value, args...);
		RequireSpace(maxLen);
		ToString(mBufferRest, Forward<T>(value), Forward<Args>(args)...);
		if(mFormatRest != null) WriteNextPart();
		return *this;
	}

	template<typename T, typename... Args> Requires<
		!CCopyConstructible<TRemoveConstRef<T>> &&
		!CConst<T> &&
		!(!CInputRange<T> && CHasRangeOf<T>),
	StringFormatter&> operator()(T&& value, Args&&... args)
	{
		DiscardBuffer();
		ToString(LastAppender(mResult), Forward<T>(value), Forward<Args>(args)...);
		if(mFormatRest != null) WriteNextPart();
		return *this;
	}

	template<typename T, typename... Args> forceinline Requires<
		!CInputRange<T> &&
		CHasRangeOf<T>,
	StringFormatter&> operator()(T&& value, Args&&... args)
	{return operator()(ForwardAsRange<T>(value), Forward<Args>(args)...);}

	template<size_t N> forceinline StringFormatter& operator()(const Char(&arr)[N])
	{return operator()(GenericStringView<const Char>(arr));}

	forceinline StringFormatter& operator()(const Char* cstr)
	{return operator()(GenericStringView<const Char>(cstr));}

	template<typename... Args> StringFormatter& Arg(Args&&... args)
	{
		TExpand{(operator()(Forward<Args>(args)), '\0')...};
		return *this;
	}
};
INTRA_END
