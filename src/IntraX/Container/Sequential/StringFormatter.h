#pragma once

#include "Intra/CContainer.h"

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Inserter.h"
#include "Intra/Range/Stream/ToStringArithmetic.h"
#include "Intra/Range/Stream/ToString.h"
#include "Intra/Range/Stream/MaxLengthOfToString.h"
#include "Intra/Range/Search/Subrange.h"

#include "IntraX/Container/Operations.hh"

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_SIGN_CONVERSION
//TODO: replace this with complete formatting solution with positional and named argument support and argument formatting specification in a format string.

template<typename S> class StringFormatter
{
	static_assert(CHas_data<S>, "S must contain data method!");
	static_assert(CHas_size<S>, "S must contain size method!");
	static_assert(CGrowingList<S>, "S must be a sequential container!");
	typedef TArrayListValue<S> Char;
public:
	operator S()
	{
		INTRA_PRECONDITION(mFormatRest.Empty());
		DiscardBuffer();
		return Move(mResult);
	}
	INTRA_FORCEINLINE S operator*() {return operator S();}

private:
	void WriteNextPart()
	{
		RequireSpace(mFormatRest.Length());
		const auto partLen = CountUntil(mFormatRest, "<^>");
		const auto charsWritten = WriteTo(mFormatRest.Take(partLen), mBufferRest);
		mFormatRest.PopFirstCount(charsWritten+3);
	}

	void RequireSpace(index_t newChars)
	{
		if(mBufferRest.Length() >= newChars) return; //No need to reallocate buffer
		const index_t currentLength = CurrentLength();
		auto newSize = currentLength + Max(currentLength, newChars);
		if(newSize < 16) newSize = 16;
		SetCountTryNotInit(mResult, newSize);
		mBufferRest = Drop(mResult, currentLength);
	}

	index_t CurrentLength() const
	{return index_t(mResult.size()) - mBufferRest.Length();}

	void DiscardBuffer()
	{
		SetCountTryNotInit(mResult, CurrentLength());
		mBufferRest = nullptr;
	}

	GenericStringView<const Char> mFormatRest;
	S mResult;
	GenericStringView<Char> mBufferRest;

	StringFormatter(const StringFormatter&) = delete;
	StringFormatter& operator=(const StringFormatter&) = delete;

public:
	StringFormatter(GenericStringView<const Char> formatStr):
		mFormatRest(formatStr),
		mResult(), mBufferRest(nullptr)
	{
		if(mFormatRest != nullptr)
			WriteNextPart();
	}

	StringFormatter(StringFormatter&& rhs):
		mFormatRest(rhs.mFormatRest), mResult(Move(rhs.mResult)),
		mBufferRest(rhs.mBufferRest) {rhs.mBufferRest = nullptr;}

	~StringFormatter() {INTRA_PRECONDITION(mFormatRest.Empty());}

	template<typename T, typename... Args> Requires<
		CCopyConstructible<TRemoveConstRef<T>> &&
		!(!CRange<T> && CHasRangeOf<T>),
	StringFormatter&> operator()(T&& value, Args&&... args)
	{
		const size_t maxLen = MaxLengthOfToString(value, args...);
		RequireSpace(maxLen);
		ToString(mBufferRest, Forward<T>(value), Forward<Args>(args)...);
		if(mFormatRest != nullptr) WriteNextPart();
		return *this;
	}

	template<typename T, typename... Args> Requires<
		!CCopyConstructible<TRemoveConstRef<T>> &&
		!CConst<T> &&
		!(!CRange<T> && CHasRangeOf<T>),
	StringFormatter&> operator()(T&& value, Args&&... args)
	{
		DiscardBuffer();
		ToString(LastAppender(mResult), Forward<T>(value), Forward<Args>(args)...);
		if(mFormatRest != nullptr) WriteNextPart();
		return *this;
	}

	template<typename T, typename... Args> Requires<
		!CRange<T> &&
		CHasRangeOf<T>,
	StringFormatter&> operator()(T&& value, Args&&... args)
	{return operator()(ForwardAsRange<T>(value), Forward<Args>(args)...);}

	template<size_t N> StringFormatter& operator()(const Char(&arr)[N])
	{return operator()(GenericStringView<const Char>(arr));}

	StringFormatter& operator()(const Char* cstr)
	{return operator()(GenericStringView<const Char>(cstr));}

	template<typename... Args> StringFormatter& Arg(Args&&... args)
	{
		(void)TExpand{(operator()(Forward<Args>(args)), '\0')...};
		return *this;
	}
};
} INTRA_END
