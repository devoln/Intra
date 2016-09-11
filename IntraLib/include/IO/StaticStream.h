#pragma once

#include "Meta/Type.h"
#include "Algorithms/Range.h"
#include "Algorithms/RangeIteration.h"
#include "Algorithms/RangeConstruct.h"
#include "Containers/StringView.h"
#include "Algorithms/AsciiString.h"

namespace Intra { namespace IO {

class MemoryOutput
{
public:
	MemoryOutput(ArrayRange<byte> dstBuf):
		Rest((char*)dstBuf.Begin, (char*)dstBuf.End), Begin((char*)dstBuf.Begin) {}

	MemoryOutput(ArrayRange<char> dstBuf):
		Rest(dstBuf), Begin(dstBuf.Begin) {}

	MemoryOutput(ArrayRange<char> dstBuf, char* pos):
		Rest(pos, dstBuf.End), Begin(dstBuf.Begin)
	{
		INTRA_ASSERT(pos>=dstBuf.Begin && pos<=dstBuf.End);
	}

	void WriteRaw(const void* src, size_t bytes)
	{
		INTRA_ASSERT(Begin <= Rest.Begin);
		INTRA_ASSERT(bytes <= Rest.Length());
		core::memcpy(Rest.Begin, src, bytes);
		Rest.Begin += bytes;
	}

	forceinline void WriteRaw(StringView src) {WriteRaw(src.Data(), src.Length());}
	
	template<typename T> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_
	> WriteRaw(const T& src) {WriteRaw(&src, sizeof(T));}

	forceinline void WriteShortRaw(StringView src)
	{
		INTRA_ASSERT(Begin<=Rest.Begin);
		INTRA_ASSERT(src.Length()<=Rest.Length());
		auto srcPtr = src.Data(), srcEnd = src.End();
		while(srcPtr<srcEnd) *Rest.Begin++ = *srcPtr++;
	}

	//! Вспомогательная функция для текстовой сериализации, которая просто записывает символ
	forceinline void WriteCharRaw(char c)
	{
		INTRA_ASSERT(Begin<=Rest.Begin);
		INTRA_ASSERT(!Rest.Empty());
		Rest.Put(c);
	}

	forceinline void WriteFloatText(real v, short preciseness, char decimalSeparator)
	{
		INTRA_ASSERT(Begin<=Rest.Begin);
		//INTRA_ASSERT(Algo::LengthOfNumber(v, preciseness)<=Rest.Length());
		Rest.AppendAdvance(v, preciseness, decimalSeparator);
	}

	template<typename T> forceinline void WriteIntegerText(T v)
	{
		INTRA_ASSERT(Begin<=Rest.Begin);
		//INTRA_ASSERT(Algo::LengthOfNumber(v, 10u)<=Rest.Length());
		Rest.AppendAdvance(v);
	}

	forceinline void WriteReplacedString(StringView str,
		ArrayRange<const StringView> fromSubStrs, ArrayRange<const StringView> toSubStrs)
	{
		INTRA_ASSERT(Begin <= Rest.Begin);
		Algo::StringMultiReplaceAscii(str, Rest, fromSubStrs, toSubStrs);
	}

	forceinline StringView GetString() const
	{
		INTRA_ASSERT(Begin <= Rest.Begin);
		return StringView(Begin, Rest.Begin);
	}

	forceinline size_t BytesWritten() const
	{
		INTRA_ASSERT(Begin <= Rest.Begin);
		return size_t(Rest.Begin - Begin);
	}

	forceinline ArrayRange<const char> GetRange() const
	{
		INTRA_ASSERT(Begin <= Rest.Begin);
		return ArrayRange<const char>(Begin, Rest.Begin);
	}

	ArrayRange<char> Rest;
	char* Begin;
};

class DummyOutput
{
public:
	DummyOutput() {}

	forceinline void WriteRaw(StringView src) {counter.Counter += src.Length();}
	forceinline void WriteRaw(const void* src, size_t bytes) {(void)src; counter.Counter += bytes;}
	template<typename T> forceinline Meta::EnableIfPod<T> WriteRaw(const T& src) {src; counter.Counter += sizeof(T);}
	forceinline void WriteShortRaw(StringView src) {counter.Counter += src.Length();}

	//! Вспомогательная функция для текстовой сериализации, которая просто записывает символ
	forceinline void WriteCharRaw(char src) {(void)src; counter.Counter++;}

	forceinline void WriteFloatText(real v, int preciseness, char decimalSeparator)
	{
		counter.AppendAdvance(v, preciseness, decimalSeparator);
	}

	template<typename T> forceinline void WriteIntegerText(T v)
	{
		counter.AppendAdvance(v);
	}

	forceinline void WriteReplacedString(StringView str,
		ArrayRange<const StringView> fromSubStrs, ArrayRange<const StringView> toSubStrs)
	{
		counter.Counter += Algo::StringMultiReplaceAsciiLength(str, fromSubStrs, toSubStrs);
	}

	forceinline size_t BytesWritten() const {return counter.Counter;}

private:
	Range::CountRange<char> counter;
};



class MemoryInput
{
public:
	MemoryInput(StringView str):
		Begin(str.Data()), Rest(str) {}

	MemoryInput(ArrayRange<const char> srcBuf):
		Begin(srcBuf.Begin), Rest(srcBuf.Begin, srcBuf.End) {}

	MemoryInput(ArrayRange<const char> srcBuf, const char* pos):
		Begin(srcBuf.Begin), Rest(pos, srcBuf.End)
	{
		INTRA_ASSERT(srcBuf.Begin <= pos && pos<=srcBuf.End);
	}

	void ReadRaw(void* dst, size_t bytes)
	{
		INTRA_ASSERT(Begin <= Rest.Data());
		INTRA_ASSERT(Rest.Length() >= bytes);
		core::memcpy(dst, Rest.Data(), bytes);
		Rest.PopFirstN(bytes);
	}

	template<typename T> forceinline Meta::EnableIfPod<T> ReadRaw(T& dst) {ReadRaw(&dst, sizeof(T));}

	forceinline void SkipSpaces()
	{
		Rest.TrimLeftAdvance(Range::IsHorSpace<char>);
	}

	forceinline uint SkipAllSpaces()
	{
		return (uint)Rest.SkipSpacesCountLinesAdvance();
	}


	forceinline bool Expect(StringView str)
	{
		if(!Rest.StartsWith(str)) return false;
		Rest.PopFirstN(str.Length());
		return true;
	}

	forceinline bool ExpectSkipSpaces(StringView str)
	{
		SkipSpaces();
		return Expect(str);
	}


	//! Читает один символ
	forceinline char ReadCharRaw()
	{
		INTRA_ASSERT(Rest.Data() >= Begin);
		char result = Rest.First();
		Rest.PopFirst();
		return result;
	}

	template<typename T> forceinline Meta::EnableIf<Meta::IsFloatType<T>::_, T> Parse(char decimalSeparator='.')
	{
		INTRA_ASSERT(Rest.Data() >= Begin);
		return Rest.ParseAdvance<T>(decimalSeparator);
	}

	template<typename T> forceinline Meta::EnableIf<Meta::IsIntegralType<T>::_, T> Parse()
	{
		INTRA_ASSERT(Rest.Data() >= Begin);
		return Rest.ParseAdvance<T>();
	}

	forceinline StringView ParseIdentifier()
	{
		static const AsciiSet notFirstChars = AsciiSet::NotIdentifierChars|AsciiSet::Digits;
		return Rest.ParseIdentifierAdvance(notFirstChars, AsciiSet::NotIdentifierChars);
	}

	forceinline StringView ReadUntilChar(const AsciiSet& stopCharset)
	{
		static_assert(!Range::IsFiniteForwardRangeOf<AsciiSet, char>::_, "ERROR!!!");
		static_assert(Meta::IsCallable<AsciiSet, char>::_, "ERROR!!!");
		static_assert(!Meta::IsConvertible<AsciiSet, char>::_, "ERROR!!!");
		return Rest.ReadUntilAdvance(stopCharset);
	}

	forceinline StringView ReadUntil(StringView stopStr)
	{
		return Rest.ReadUntilAdvance(stopStr);
	}

	forceinline StringView ReadRecursiveBlock(int& counter,
		StringView openingBracket="{", StringView closingBracket="}", StringView stopToken=null,
		ArrayRange<const core::pair<StringView, StringView>> commentBlocks={{"//", "\n"},{"/*", "*/"}},
		ArrayRange<const core::pair<StringView, StringView>> recursiveCommentBlocks=null)
	{
		return Rest.ReadRecursiveBlockAdvance(counter, null,
			openingBracket, closingBracket, stopToken,
			commentBlocks, recursiveCommentBlocks);
	}

	forceinline size_t Position() const
	{
		INTRA_ASSERT(Rest.Data() >= Begin);
		return size_t(Rest.Data()-Begin);
	}

	forceinline StringView GetString() const {return Rest;}

	const char* Begin;
	StringView Rest;
};


}}
