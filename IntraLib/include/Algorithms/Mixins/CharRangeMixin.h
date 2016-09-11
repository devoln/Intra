#pragma once

#include "Core/Core.h"
#include "Meta/Type.h"
#include "Algorithms/RangeConcept.h"

#include "Algorithms/Mixins/InputRangeMixin.h"
#include "Algorithms/Mixins/ForwardRangeMixin.h"
#include "Algorithms/Mixins/BidirectionalRangeMixin.h"
#include "Algorithms/Mixins/RandomAccessRangeMixin.h"
#include "Algorithms/Mixins/ArrayRangeMixin.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4512)
#pragma warning(disable: 4610)
#endif

namespace Intra { namespace Range {

template<typename T> struct SimpleArrayRange:
	ArrayRangeMixin<SimpleArrayRange<T>, Meta::RemoveConst<T>,
	FiniteRandomAccessRangeMixin<SimpleArrayRange<T>, Meta::RemoveConst<T>,
	RandomAccessRangeMixin<SimpleArrayRange<T>, Meta::RemoveConst<T>,
	BidirectionalRangeMixin<SimpleArrayRange<T>, Meta::RemoveConst<T>,
	FiniteForwardRangeMixin<SimpleArrayRange<T>, Meta::RemoveConst<T>,
	ForwardRangeMixin<SimpleArrayRange<T>, Meta::RemoveConst<T>,
	FiniteInputRangeMixin<SimpleArrayRange<T>, Meta::RemoveConst<T>,
	FiniteInputRangeMixin<SimpleArrayRange<T>, Meta::RemoveConst<T>, Meta::EmptyType
	>>>>>>>>
{
	T* Begin;
	T* End;

	enum {RangeType=TypeEnum::Array};
	enum: bool {RangeIsFinite=true};
	typedef Meta::RemoveConst<T> value_type;
	typedef T& return_value_type;

	SimpleArrayRange(null_t=null): Begin(null), End(null) {}
	SimpleArrayRange(T* begin, T* end): Begin(begin), End(end) {}
	template<size_t N> SimpleArrayRange(T(&arr)[N]): Begin(arr), End(arr+N) {}
	template<size_t N> SimpleArrayRange(T(&arr)[N], null_t): Begin(arr), End(arr+N-1) {}


	forceinline T& First() const {INTRA_ASSERT(!Empty()); return *Begin;}
	forceinline T& Last() const {INTRA_ASSERT(!Empty()); return *(End-1);}
	forceinline void PopFirst() {INTRA_ASSERT(!Empty()); Begin++;}
	forceinline void PopLast() {INTRA_ASSERT(!Empty()); End--;}
	forceinline bool Empty() const {return Begin==End;}
	forceinline size_t Length() const {return End-Begin;}
	forceinline T& operator[](size_t index) const {INTRA_ASSERT(index<Length()); return Begin[index];}
	forceinline T* Data() const {return Begin;}
	forceinline T* begin() const {return Begin;}
	forceinline T* end() const {return End;}
	forceinline SimpleArrayRange opSlice(size_t start, size_t end) const {return {Begin+start, Begin+end};}
};

template<typename Char> forceinline Meta::EnableIf<
	Meta::IsCharType<Char>::_,
Char> ToLowerAscii(Char c)
{
	if(unsigned(c-'A')>'Z'-'A') return c;
	return c+('a'-'A');
}

template<typename Char> forceinline Meta::EnableIf<
	Meta::IsCharType<Char>::_,
Char> ToUpperAscii(Char c)
{
	if(unsigned(c-'a')>'z'-'a') return c;
	return c-('a'-'A');
}

template<typename Char> forceinline bool IsHorSpace(Char c) {return c==' ' || c=='\t';}
template<typename Char> forceinline bool IsSpace(Char c) {return IsHorSpace(c) || c=='\r' || c=='\n';}

template<typename T> struct CountRange;

template<typename R, typename T> struct CharRangeMixin
{
private:
	forceinline const R& me() const {return *static_cast<const R*>(this);}
	forceinline R& me() {return *static_cast<R*>(this);}

	template<typename Range, typename OtherCharRange> struct TupleAppender
	{
		bool first;
		Range& range;
		const OtherCharRange& separator;

		template<typename V> void operator()(const V& value)
		{
			if(!first) range.AppendAdvance(separator);
			range.AppendAdvance(value);
			first = false;
		}
	};

public:
	void AppendCStrAdvance(const char* str)
	{
		while(*str!='\0') me().Put(*str++);
	}

	template<typename X> Meta::EnableIf<
		Meta::IsUnsignedIntegralType<X>::_,
	R&> AppendAdvance(X number, int minWidth, T filler=' ', uint base=10, T minus='\0')
	{
		INTRA_ASSERT(base>=2 && base<=36);
		T reversed[64];
		T* rev = reversed;
		do *rev++ = "0123456789abcdefghijklmnopqrstuvwxyz"[number%base], number = X(number/base);
		while(number!=0);
		if(minus) minWidth--;
		for(int i=0, s=int(minWidth-(rev-reversed)); i<s; i++)
			me().Put(filler);
		if(minus) me().Put(minus);
		while(rev!=reversed) me().Put(*--rev);
		return me();
	}

	template<typename X> static Meta::EnableIf<
		Meta::IsUnsignedIntegralType<X>::_,
	size_t> MaxLengthOf(X number, int minWidth, T filler=' ', uint base=10, T minus='\0')
	{
		(void)number; (void)filler;
		size_t maxLog;
		if(base<8) maxLog = sizeof(X)*8;
		else if(base<16) maxLog = (sizeof(X)*8+2)/3;
		else maxLog = sizeof(X)*2;
		return Math::Max(maxLog+(minus!='\0'), size_t(minWidth));
	}

	template<typename X> Meta::EnableIf<
		Meta::IsUnsignedIntegralType<X>::_ && sizeof(X)>=sizeof(size_t),
	R&> AppendAdvance(X number)
	{
		T reversed[20];
		T* rev = reversed;
		do *rev++ = T(number%10+'0'), number/=10;
		while(number!=0);
		while(rev!=reversed)
			me().Put(*--rev);
		return me();
	}

	template<typename X> Meta::EnableIf<
		Meta::IsUnsignedIntegralType<X>::_ && sizeof(X)<sizeof(size_t),
	R&> AppendAdvance(X number)
	{
		return AppendAdvance(size_t(number));
	}

	template<typename X> static forceinline Meta::EnableIf<
		Meta::IsUnsignedIntegralType<X>::_,
	size_t> MaxLengthOf(X number)
	{
		(void)number;
		return sizeof(X)<2? 3: sizeof(X)==2? 5: sizeof(X)<=4? 10: sizeof(X)<=8? 18: 35;
	}

	template<typename X> forceinline Meta::EnableIf<
		Meta::IsSignedIntegralType<X>::_,
	R&> AppendAdvance(X number, int minWidth, T filler=' ', uint base=10)
	{
		return me().AppendAdvance(Meta::MakeUnsignedType<X>(number<0? -number: number), minWidth, filler, base, number<0? '-': '\0');
	}


	template<typename X> static forceinline Meta::EnableIf<
		Meta::IsSignedIntegralType<X>::_,
	size_t> MaxLengthOf(X number, int minWidth, T filler=' ', uint base=10)
	{
		return 1+MaxLengthOf(Meta::MakeUnsignedType<X>(number<0? -number: number), minWidth, filler, base, number<0? '-': '\0');
	}

	template<typename X> Meta::EnableIf<
		Meta::IsSignedIntegralType<X>::_,
	R&> AppendAdvance(X number)
	{
		if(number<0)
		{
			me().Put('-');
			number = X(-number);
		}
		return me().AppendAdvance(Meta::MakeUnsignedType<X>(number));
	}

	template<typename X> static forceinline Meta::EnableIf<
		Meta::IsSignedIntegralType<X>::_,
	size_t> MaxLengthOf(X number)
	{
		(void)number;
		return sizeof(X)<2? 4: sizeof(X)==2? 6: sizeof(X)<=4? 11: sizeof(X)<=8? 18: 35;
	}

	template<typename X> Meta::EnableIf<
		Meta::IsUnsignedIntegralType<X>::_,
	R&> AppendHexIntAdvance(X number)
	{
		intptr digitPos = sizeof(X)*2;
		while(digitPos --> 0)
		{
			int value = (number >> (digitPos*4)) & 15;
			if(value>9) value += 'A'-10;
			else value += '0';
			me().Put(T(value));
		}
		return me();
	}


	template<typename X> forceinline R& AppendAdvance(X* pointer)
	{
		return me().AppendHexIntAdvance((size_t)pointer);
	}

	template<typename X> static forceinline size_t MaxLengthOf(X* pointer) {(void)pointer; return sizeof(X*)*2;}



	template<typename X> Meta::EnableIf<
		Meta::IsFloatType<X>::_ && !Meta::TypeEquals<X, real>::_,
	R&> AppendAdvance(X number, int preciseness=sizeof(X)<=4? 7: 15, T dot='.', bool appendAllDigits=false)
	{
		return AppendAdvance(real(number), preciseness, dot, appendAllDigits);
	}

	R& AppendAdvance(real number, int preciseness=15, T dot='.', bool appendAllDigits=false)
	{
		if(number==Math::NaN)
		{
			AppendCStrAdvance("NaN");
			return me();
		}
		if(number==Math::Infinity)
		{
			AppendCStrAdvance("Infinity");
			return me();
		}
		if(number==-Math::Infinity)
		{
			AppendCStrAdvance("-Infinity");
			return me();
		}

		if(number<0)
		{
			me().Put('-');
			number = -number;
		}

		const ulong64 integralPart = (ulong64)number;
		real fractional = number-integralPart;
		if(fractional>0.99)
		{
			me().AppendAdvance(integralPart+1);
			fractional=0;
		}
		else me().AppendAdvance(integralPart);

		if(preciseness==0) return me();

		me().Put(dot);
		do
		{
			fractional *= 10;
			int digit = (int)fractional;
			fractional -= digit;
			if(fractional>0.99) fractional=0, digit++;
			me().Put(T('0'+digit));
		} while((fractional>=0.01 || appendAllDigits) && --preciseness>0);
		return me();
	}

	template<typename X> static forceinline Meta::EnableIf<
		Meta::IsFloatType<X>::_,
	size_t> MaxLengthOf(X number, int preciseness=2, T dot='.', bool appendAllDigits=false)
	{
		(void)number; (void)dot; (void)appendAllDigits;
		return 20+1+(preciseness+1);
	}


	template<typename Char2> forceinline Meta::EnableIf<
		Meta::IsCharType<Char2>::_,
	R&> AppendAdvance(Char2 character, size_t repeat=1)
	{
		while(repeat --> 0) me().Put((T)character);
		return me();
	}

	template<typename Char2> static forceinline Meta::EnableIf<
		Meta::IsCharType<Char2>::_,
	size_t> MaxLengthOf(Char2 character, size_t repeat=1) {(void)character; return repeat;}


	template<typename InputRange> forceinline Meta::EnableIf<
		IsFiniteInputRangeOfExactly<InputRange, T>::_,
	R&> AppendAdvance(const InputRange& range)
	{
		auto r = range;
		r.CopyAdvanceToAdvance(me());
		return me();
	}

	template<typename InputRange> static forceinline Meta::EnableIf<
		IsFiniteInputRangeOfExactly<InputRange, T>::_,
	size_t> MaxLengthOf(const InputRange& range) {return range.Count();}


	R& AppendAdvance(bool value)
	{
		static const char* const boolStr[2] = {"false", "true"};
		const char* str = boolStr[(size_t)value];
		while(*str!='\0') me().Put(*str++);
		return me();
	}

	size_t MaxLengthOf(bool value) {(void)value; return 5;}

	template<typename InputRange, typename OtherCharRange=SimpleArrayRange<const T>> Meta::EnableIf<
		IsFiniteInputNonCharRange<InputRange>::_ && IsCharRange<OtherCharRange>::_,
	R&> AppendAdvance(const InputRange& r, OtherCharRange separator=SimpleArrayRange<const T>(", ", null),
		OtherCharRange lBracket=SimpleArrayRange<const T>("[", null), OtherCharRange rBracket=SimpleArrayRange<const T>("]", null))
	{
		InputRange range = r;
		me().AppendAdvance(lBracket);
		if(!range.Empty())
		{
			me().AppendAdvance(range.First());
			range.PopFirst();
		}
		while(!range.Empty())
		{
			me().AppendAdvance(separator);
			me().AppendAdvance(range.First());
			range.PopFirst();
		}
		me().AppendAdvance(rBracket);
		return me();
	}

	template<typename ForwardRange, typename OtherCharRange=SimpleArrayRange<const T>> static forceinline Meta::EnableIf<
		IsFiniteForwardNonCharRange<ForwardRange>::_ &&
		!(IsRangeOfRanges<ForwardRange>::_ || IsRangeOfTuples<ForwardRange>::_) &&
		IsCharRange<OtherCharRange>::_,
	size_t> MaxLengthOf(const ForwardRange& range,
		const OtherCharRange& separator=SimpleArrayRange<const T>(", ", null),
		const OtherCharRange& lBracket=SimpleArrayRange<const T>("[", null),
		const OtherCharRange& rBracket=SimpleArrayRange<const T>("]", null))
	{
		return (MaxLengthOf(typename ForwardRange::value_type()) + separator.Count())*range.Count() +
			lBracket.Count() + rBracket.Count();
	}

	template<typename ForwardRange, typename OtherCharRange=SimpleArrayRange<const T>> static forceinline Meta::EnableIf<
		IsFiniteForwardNonCharRange<ForwardRange>::_ &&
		(IsRangeOfRanges<ForwardRange>::_ || IsRangeOfTuples<ForwardRange>::_) &&
		IsCharRange<OtherCharRange>::_,
	size_t> MaxLengthOf(const ForwardRange& range,
		const OtherCharRange& separator=SimpleArrayRange<const T>(", ", null),
		const OtherCharRange& lBracket=SimpleArrayRange<const T>("[", null),
		const OtherCharRange& rBracket=SimpleArrayRange<const T>("]", null))
	{
		size_t len=0;
		auto rangeCopy = range;
		while(!rangeCopy.Empty())
		{
			len += MaxLengthOf(rangeCopy.First());
			rangeCopy.PopFirst();
		}
		return len + separator.Count()*range.Count() + lBracket.Count() + rBracket.Count();
	}


	template<typename Tuple, typename OtherCharRange=SimpleArrayRange<const T>> Meta::EnableIf<
		Meta::IsTuple<Tuple>::_ &&
		IsCharRange<OtherCharRange>::_,
	R&> AppendAdvance(const Tuple& tuple,
		const OtherCharRange& separator=SimpleArrayRange<const T>(", ", null),
		const OtherCharRange& lBracket=SimpleArrayRange<const T>("{", null),
		const OtherCharRange& rBracket=SimpleArrayRange<const T>("}", null))
	{
		me().AppendAdvance(lBracket);
		TupleAppender<R, OtherCharRange> appender{true, me(), separator};
		tuple.ForEachField(appender);
		me().AppendAdvance(rBracket);
		return me();
	}

	template<typename Tuple, typename OtherCharRange=SimpleArrayRange<const T>> static Meta::EnableIf<
		Meta::IsTuple<Tuple>::_ &&
		IsCharRange<OtherCharRange>::_,
	size_t> MaxLengthOf(const Tuple& tuple,
		const OtherCharRange& separator=SimpleArrayRange<const T>(", ", null),
		const OtherCharRange& lBracket=SimpleArrayRange<const T>("{", null),
		const OtherCharRange& rBracket=SimpleArrayRange<const T>("}", null))
	{
		CountRange<T> counter;
		counter.AppendAdvance(lBracket);
		TupleAppender<CountRange<T>, OtherCharRange> appender{false, counter, separator};
		tuple.ForEachField(appender);
		counter.AppendAdvance(rBracket);
		return counter.Counter;
	}




	template<typename X, typename OtherCharRange=SimpleArrayRange<const T>> Meta::EnableIf<
		!IsInputRange<X>::_ && Meta::IsClass<X>::_ && !Meta::IsTuple<X>::_,
	R&> AppendAdvance(const X& value)
	{
		return me().AppendAdvance(value.AsRange());
	}

	template<typename X, typename OtherCharRange=SimpleArrayRange<const T>> static forceinline Meta::EnableIf<
		!IsInputRange<X>::_ && Meta::IsClass<X>::_ && !Meta::IsTuple<X>::_,
	size_t> MaxLengthOf(const X& value) {return MaxLengthOf(value.AsRange());}



	bool ParseSignAdvance()
	{
		bool minus = false;
		while(!me().Empty())
		{
			if(me().First()=='-') minus = !minus;
			else if(me().First()!='+' && !IsSpace(me().First())) break;
			me().PopFirst();
		}
		return minus;
	}

	template<typename X> Meta::EnableIf<
		Meta::IsUnsignedIntegralType<X>::_,
	X> ParseAdvance()
	{
		me().TrimLeftAdvance(IsHorSpace<T>);
		X result=0;
		while(!me().Empty())
		{
			uint digit = uint(me().First())-'0';
			if(digit>9) break;
			result = X(result*10+digit);
			me().PopFirst();
		}
		return result;
	}

	template<typename X> Meta::EnableIf<
		Meta::IsSignedIntegralType<X>::_,
	X> ParseAdvance()
	{
		const bool minus = me().ParseSignAdvance();
		X result = (X)ParseAdvance<Meta::MakeUnsignedType<X>>();
		return minus? X(-result): result;
	}

	template<typename X> Meta::EnableIf<
		Meta::IsFloatType<X>::_,
	X> ParseAdvance(T decimalSeparator='.')
	{
		X result = 0, pos = 1;
		bool waspoint = false;

		bool minus = ParseSignAdvance();

		for(; !me().Empty(); me().PopFirst())
		{
			T c = me().First();
			if(c==decimalSeparator && !waspoint)
			{
				waspoint=true;
				continue;
			}
			uint digit = uint(c)-'0';
			if(digit>9) break;

			if(!waspoint) result = result*10+digit;
			else pos*=10, result += digit/pos;
		}
		return minus? -result: result;
	}


	size_t SkipSpacesCountLinesAdvance()
	{
		size_t lines = 0;
		bool wasCR = false;
		while(!me().Empty() && IsSpace(me().First()))
		{
			char c = me().First();
			if(c=='\r' || (c=='\n' && !wasCR))
				lines++;
			wasCR = (c=='\r');
			me().PopFirst();
		}
		return lines;
	}

	size_t CountLinesAdvance()
	{
		size_t lines = 0;
		bool wasCR = false;
		while(!me().Empty())
		{
			char c = me().First();
			if(c=='\r' || (c=='\n' && !wasCR))
				lines++;
			wasCR = c=='\r';
			me().PopFirst();
		}
		return lines;
	}

	forceinline size_t CountLines()
	{
		R range = me();
		return range.CountLinesAdvance();
	}

};





}}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
