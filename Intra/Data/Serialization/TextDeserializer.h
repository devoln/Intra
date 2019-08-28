#pragma once

#include "Core/Type.h"
#include "Core/Tuple.h"
#include "Core/Pair.h"
#include "Core/EachField.h"

#include "Core/Range/StringView.h"
#include "Utils/AsciiSet.h"

#include "Core/CContainer.h"

#include "Core/Range/Search/RecursiveBlock.h"
#include "Core/Range/Comparison/EndsWith.h"
#include "Core/Range/TakeUntil.h"
#include "Core/Range/TakeUntilAny.h"
#include "Core/Range/Stream/Parse.h"

#include "Data/Reflection.h"
#include "LanguageParams.h"

#include "Container/Sequential/String.h"
#include "Container/Operations.hh"



INTRA_BEGIN
namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename I> struct GenericTextDeserializer;

template<typename I> struct GenericTextDeserializerStructVisitor
{
	GenericTextDeserializer<I>* Me;
	CSpan<StringView> FieldNames;
	bool Began;
	TextSerializerParams::TypeFlags Type;

	template<typename T> GenericTextDeserializerStructVisitor& operator()(T& t);
};

template<typename I> struct GenericTextDeserializer
{
	GenericTextDeserializer(const LanguageParams& langParams, const I& input):
		Lang(langParams), NestingLevel(0), Line(0), Input(input), Log() {}


	bool IgnoreField()
	{
		int counter=1;
		Range::TakeRecursiveBlockAdvance(Input, counter, null,
			Lang.StructInstanceOpen, Lang.StructInstanceClose, Lang.FieldSeparator,
		CSpan<Pair<StringView, StringView>>{
			{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
			{Lang.OneLineCommentBegin, "\n"},
			{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
			{Lang.ArrayOpen, Lang.ArrayClose}
		},
		CSpan<Pair<StringView, StringView>>{});
		INTRA_DEBUG_ASSERT(counter==0 || counter==1);
		return counter==0;
	}

	bool IgnoreArrayElement()
	{
		int counter = 1;
		auto inputCopy = Input;
		size_t charsRead = 0;
		Range::TakeRecursiveBlockAdvance(Input, counter, &charsRead,
			StringView(), Lang.ArrayClose, Lang.ArrayElementSeparator,
		CSpan<Pair<StringView, StringView>>{
			{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
			{Lang.OneLineCommentBegin, StringView("\n")},
			{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd}
		},
		CSpan<Pair<StringView, StringView>>{
			{Lang.StructInstanceOpen, Lang.StructInstanceClose},
			{Lang.ArrayOpen, Lang.ArrayClose}
		});
		if(counter==0)
		{
			Input = Move(inputCopy);
			Range::PopFirstExactly(Input, charsRead-Lang.ArrayClose.Length());
			return true;
		}
		INTRA_DEBUG_ASSERT(counter==1);
		return false;
	}

	bool NextField(TextSerializerParams::TypeFlags typeFlag)
	{
		if(typeFlag & TextSerializerParams::TypeFlags_Struct)
		{
			INTRA_DEBUG_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Struct);
			if(!Expect(Lang.FieldSeparator))
			{
				if(!Input.Empty()) IgnoreField();
				return false;
			}
		}
		if(typeFlag & TextSerializerParams::TypeFlags_Tuple)
		{
			INTRA_DEBUG_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Tuple);
			if(!Expect(Lang.TupleFieldSeparator))
			{
				if(!Input.Empty()) IgnoreField();
				return false;
			}
		}
		if(typeFlag & TextSerializerParams::TypeFlags_Array)
		{
			INTRA_DEBUG_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Array);
			if(!Expect(Lang.ArrayElementSeparator))
				return !Input.Empty() && !IgnoreArrayElement();
		}
		if(typeFlag & TextSerializerParams::TypeFlags_StructArray)
		{
			INTRA_DEBUG_ASSERT(typeFlag==TextSerializerParams::TypeFlags_StructArray);
			if(!Expect(Lang.ArrayElementSeparator))
				return !Input.Empty() && !IgnoreArrayElement();
		}
		SkipAllSpacesAndComments();
		return true;
	}

	Range::TakeResult<I> ParseIdentifier()
	{
		static const AsciiSet isNotIdentifierFirstChar = AsciiSets.NotIdentifierChars | AsciiSets.Digits;
		return Range::ParseIdentifierAdvance(Input,
			isNotIdentifierFirstChar, AsciiSets.NotIdentifierChars);
	}

	//! Прочитать имя поля и пропустить оператор присваивания, перейдя к правой части присваивания.
	//! Если в процессе обнаруживается ошибка, функция возвращает на исходную позицию и возвращает null.
	//! Иначе возвращает прочитанное имя поля.
	StringView FieldAssignmentBeginning()
	{
		auto oldPos = Input;
		auto oldLine = Line;
		StringView name;
		SkipAllSpacesAndComments();
		if(!Range::ExpectAdvance(Input, Lang.LeftFieldNameBeginQuote))
			goto error;
		SkipAllSpacesAndComments();
		name = ParseIdentifier();
		SkipAllSpacesAndComments();
		if(!Range::ExpectAdvance(Input, Lang.LeftFieldNameEndQuote))
			goto error;
		SkipAllSpacesAndComments();
		if(!Range::ExpectAdvance(Input, Lang.LeftAssignmentOperator))
			goto error;
		return name;

	error:
		Input = oldPos;
		Line = oldLine;
		return null;
	}

	StringView FieldAssignmentEnding()
	{
		const auto oldPos = Input;
		const size_t oldLine = Line;
		StringView name;
		SkipAllSpacesAndComments();
		if(!Range::ExpectAdvance(Input, Lang.RightAssignmentOperator))
			goto error;
		SkipAllSpacesAndComments();
		if(!Range::ExpectAdvance(Input, Lang.RightFieldNameBeginQuote))
			goto error;
		SkipAllSpacesAndComments();
		name = ParseIdentifier();
		SkipAllSpacesAndComments();
		if(!Range::ExpectAdvance(Input, Lang.RightFieldNameEndQuote))
			goto error;
		return name;

	error:
		Input = oldPos;
		Line = oldLine;
		return null;
	}

	void StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type)
	{
		const StringView openStr = (type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceOpen: Lang.TupleOpen;
		if(Expect(openStr))
		{
			NestingLevel++;
			return;
		}

		if(Input.Empty()) return;
		int counter = -1;
		Range::TakeRecursiveBlockAdvance(Input, counter, null,
			Lang.StructInstanceOpen, StringView(), StringView(),
			CSpan<Pair<StringView, StringView>>{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			CSpan<Pair<StringView, StringView>>{
				{Lang.ArrayOpen, Lang.ArrayClose}
			}
		);
		NestingLevel++;
	}

	void StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type)
	{
		NestingLevel--;
		const StringView closeStr = (type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceClose: Lang.TupleClose;
		if(Expect(closeStr)) return;
		int counter = 1;
		Range::TakeRecursiveBlockAdvance(Input, counter, null,
			Lang.StructInstanceOpen, Lang.StructInstanceClose, StringView(),
			CSpan<Pair<StringView, StringView>>{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			CSpan<Pair<StringView, StringView>>{
				{Lang.ArrayOpen, Lang.ArrayClose}
			}
		);
	}

	forceinline void SkipAllSpaces()
	{Line += Range::SkipSpacesCountLinesAdvance(Input);}

	void SkipAllSpacesAndComments()
	{
		for(;;)
		{
			SkipAllSpaces();
			if(!Lang.OneLineCommentBegin.Empty() && Range::ExpectAdvance(Input, Lang.OneLineCommentBegin))
			{
				int counter = 1;
				StringView commentBlock = Range::TakeRecursiveBlockAdvance(Input, counter, null,
					Lang.OneLineCommentBegin, "\n", StringView());
				Line += Range::CountLinesAdvance(commentBlock);
				continue;
			}
			if(!Lang.MultiLineCommentBegin.Empty() && Range::ExpectAdvance(Input, Lang.MultiLineCommentBegin))
			{
				int counter = 1;
				StringView commentBlock = Range::TakeRecursiveBlockAdvance(Input, counter, null,
					Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd, StringView());
				Line += Range::CountLinesAdvance(commentBlock);
				continue;
			}
			break;
		}
	}
	
	bool Expect(StringView stringToExpect)
	{
		SkipAllSpacesAndComments();
		if(Range::ExpectAdvance(Input, stringToExpect)) return true;
		if(Input.Empty())
		{
			static const StringView eofStr = "Unexpected EOF!\r\n";
			if(!Range::EndsWith(Log, eofStr)) Log += eofStr;
			return false;
		}
		LogExpectError(Range::TakeUntil(Input, AsciiSets.Spaces), {stringToExpect});
		return false;
	}

	intptr ExpectOneOf(CSpan<StringView> stringsToExpect)
	{
		SkipAllSpaces();
		size_t maxStrLength=0;
		for(size_t i=0; i<stringsToExpect.Length(); i++)
		{
			if(Range::ExpectAdvance(Input, stringsToExpect[i])) return intptr(i);
			if(maxStrLength<stringsToExpect[i].Length())
				maxStrLength = stringsToExpect[i].Length();
		}
		LogExpectError(Range::TakeUntil(Input, AsciiSets.Spaces), stringsToExpect);
		return -1;
	}

	void LogExpectError(StringView token, CSpan<StringView> expected)
	{
		char buf[1024];
		Range::OutputArrayRange<char> dst = buf;
		dst << "Error(" << Line << "): token \"" << token << "\", where expected ";
		if(expected.Length()>1) dst << "one of the folowing tokens: " << StringOf(expected, ", ", "\"", "\"");
		if(expected.Length()==1) dst << "token \"" << expected[0] << "\"";
		dst << ".\r\n";
		Log += dst.GetWrittenData();
	}


	//! Десериализовать структуру со статической информацией о полях
	template<typename T> void DeserializeStruct(T& dst, CSpan<StringView> fieldNames)
	{
		StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Struct);
		GenericTextDeserializerStructVisitor<I> visitor{this, fieldNames, false, TextSerializerParams::TypeFlags_Struct};
		if(fieldNames.Empty())
		{
			Core::ForEachField(dst, visitor);
			StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Struct);
			return;
		}

		bool expectSeparator = false;
		for(size_t i=0; ; i++)
		{
			SkipAllSpacesAndComments();
			if(Range::StartsWith(Input, Lang.StructInstanceClose)) break;

			if(expectSeparator)
			{
				if(!NextField(TextSerializerParams::TypeFlags_Struct)) break;
			}
			expectSeparator = true;

			StringView name = FieldAssignmentBeginning();

			//TODO: учесть, что имя поля может идти после его значения
			size_t index = 0;
			if(name.Empty()) index = fieldNames.Length();
			else index = Range::CountUntil(fieldNames, name);
			if(index==fieldNames.Length() && name!=null) //Если такого поля в структуре нет, пропускаем его
			{
				const bool finished = IgnoreField();
				if(!finished)
				{
					expectSeparator = false;
					continue;
				}
				NestingLevel--;
				return;
			}
			if(index!=fieldNames.Length() || name==null)
				dst.VisitFieldById(name!=null? index: i, *this);

			if(Lang.AddFieldNameAfterAssignment)
			{
				StringView name2 = FieldAssignmentEnding();
				if(name!=null && name2!=null && name!=name2) LogExpectError(name2, {name});
			}
		}
		StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Struct);
	}

	//! Десериализовать структуру со статической информацией о полях
	template<typename T> void DeserializeStruct(T& dst)
	{DeserializeStruct(dst, T::ReflectionFieldNames());}

	//! Десериализовать структуру со статической информацией о полях
	template<typename T> void DeserializeTuple(T& dst)
	{
		StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Tuple);
		GenericTextDeserializerStructVisitor<I> visitor{this, null, false, TextSerializerParams::TypeFlags_Tuple};
		Core::ForEachField(dst, visitor);
		StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Tuple);
	}

	void ConsumeArrayOpen()
	{
		if(Expect(Lang.ArrayOpen)) return;
		
		//Произошла ошибка, о которой мы уже сообщили в лог.
		//Пытаемся восстановиться. Для этого ищем начало массива на текущем уровне вложенности.
		int counter = -1;
		Range::TakeRecursiveBlockAdvance(Input, counter, null,
			Lang.ArrayOpen, StringView(), StringView(),
			CSpan<Pair<StringView, StringView>>{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			CSpan<Pair<StringView, StringView>>{
				{Lang.StructInstanceOpen, Lang.StructInstanceClose}
			}
		);
	}

	void ConsumeArrayClose()
	{
		if(Expect(Lang.ArrayClose)) return;

		//Произошла ошибка, о которой мы уже сообщили в лог.
		//Пытаемся восстановиться. Для этого ищем конец массива соответствующий текущему уровню вложенности.
		int counter = 1;
		Range::TakeRecursiveBlockAdvance(Input, counter, null,
			Lang.ArrayOpen, Lang.ArrayClose, StringView(),
			CSpan<Pair<StringView, StringView>>{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			CSpan<Pair<StringView, StringView>>{
				{Lang.StructInstanceOpen, Lang.StructInstanceClose}
			}
		);
	}

	//! Десериализовать элементы в output-диапазон
	template<typename R> void DeserializeRange(R& outputRange)
	{
		typedef TValueTypeOf<R> T;

		ConsumeArrayOpen();

		SkipAllSpacesAndComments();
		if( (Lang.ArrayClose.Empty() && !Range::StartsWith(Input, Lang.RightFieldNameBeginQuote)) ||
			!Range::StartsWith(Input, Lang.ArrayClose) )
		{
			outputRange.Put(Deserialize<T>());
			SkipAllSpacesAndComments();
			while( (Lang.ArrayClose.Empty() && !Range::StartsWith(Input, Lang.RightFieldNameBeginQuote)) ||
				!Range::StartsWith(Input, Lang.ArrayClose) )
			{
				if(NextField(TextSerializerParams::TypeFlags_Array))
					outputRange.Put(Deserialize<T>());
				else if(Input.Empty()) return;
				SkipAllSpacesAndComments();
			}
		}

		ConsumeArrayClose();
	}


	//! Десериализация вещественных чисел
	template<typename T> forceinline Requires<
		CFloatingPoint<T>::_,
	GenericTextDeserializer&> operator>>(T& v)
	{
		v = Range::ParseAdvance<T>(Input, Lang.DecimalSeparator);
		return *this;
	}

	//! Десериализация целых чисел
	template<typename T> forceinline Requires<
		CIntegral<T>::_,
	GenericTextDeserializer&> operator>>(T& v)
	{
		Input >> v;
		return *this;
	}

	//! Десериализация bool
	forceinline GenericTextDeserializer& operator>>(bool& v)
	{
		SkipAllSpaces();
		const StringView identifier = ParseIdentifier();
		if(identifier == Lang.FalseTrueNames[true])
		{
			v = true;
			return *this;
		}
		if(identifier != Lang.FalseTrueNames[false])
			LogExpectError(identifier, Lang.FalseTrueNames);
		v = false;
		return *this;
	}

	//! Десериализация кортежей
	template<typename T> forceinline Requires<
		CHasForEachField<T>::_ &&
		!HasReflectionFieldNamesMethod<T>::_,
	GenericTextDeserializer&> operator>>(T& v)
	{
		DeserializeTuple(v);
		return *this;
	}

	//! Десериализация структур
	template<typename T> forceinline Requires<
		CHasForEachField<T>::_ &&
		HasReflectionFieldNamesMethod<T>::_,
	GenericTextDeserializer&> operator>>(T& v)
	{
		DeserializeStruct(v);
		return *this;
	}

	template<typename C> Requires<
		CAsCharRange<C>::_ &&
		CHas_resize<C>::_,
	GenericTextDeserializer&> operator>>(C& dst)
	{
		if(!Expect(Lang.StringQuote)) return *this;

		auto escapedResult = Range::TakeUntilAdvance(Input, Lang.StringQuote);
		Range::PopFirstExactly(Input, Lang.StringQuote.Length());

		/*dst.SetLengthUninitialized(Range::StringMultiReplaceAsciiLength(escapedResult, {"\\n", "\\r", "\\t"}, {"\n", "\r", "\t"}));
		auto dstRange = dst.AsRange();
		Range::StringMultiReplaceAscii(escapedResult, dstRange, {"\\n", "\\r", "\\t"}, {"\n", "\r", "\t"});*/


		//TODO: Вынести экранирование строк в LanguageParams и добавить экранирование кавычек
		const KeyValuePair<StringView, StringView> replacements[] = {{"\\n", "\n"}, {"\\r", "\r"}, {"\\t", "\t"}};
		Range::CountRange<char> counter;
		Range::MultiReplaceToAdvance(escapedResult, counter, replacements);
		Core::SetCountTryNotInit(dst, counter.Counter);
		auto strData = RangeOf(dst);
		Range::MultiReplaceToAdvance(escapedResult, strData, replacements);

		return *this;
	}

	//! Десериализовать массив
	template<typename C> Requires<
		CHas_clear<C>::_ &&
		CHas_push_back<C>::_ &&
		!CChar<TValueTypeOf<C>>::_,
	GenericTextDeserializer&> operator>>(C& container)
	{
		container.clear();
		auto appender = Range::LastAppender(container);
		DeserializeRange(appender);
		return *this;
	}

	//! Десериализовать диапазон
	template<typename R> forceinline Requires<
		COutputRange<R>::_,
	GenericTextDeserializer&> operator>>(R&& range)
	{
		DeserializeRange(range);
		return *this;
	}

	//! Десериализовать массив
	template<typename T, size_t N> forceinline GenericTextDeserializer& operator>>(T(&arr)[N])
	{
		Span<T> range = arr;
		DeserializeRange(range);
		return *this;
	}


	//! Десериализовать любое значение
	template<typename T> forceinline Requires<
		!CConst<T>::_,
	GenericTextDeserializer&> operator()(T&& value) {return *this >> value;}

	//! Десериализовать любое значение
	template<typename T> forceinline T Deserialize()
	{
		T result;
		*this >> result;
		return result;
	}

	void ResetStream(const I& stream)
	{
		Input = stream;
		Line = 0;
		NestingLevel = 0;
	}

	LanguageParams Lang;
	int NestingLevel;
	size_t Line;
	I Input;
	String Log;
};

template<typename I> template<typename T> GenericTextDeserializerStructVisitor<I>&
	GenericTextDeserializerStructVisitor<I>::operator()(T& t)
{
	if(Began)
	{
		Me->NextField(Type);
		if(!FieldNames.Empty()) FieldNames.PopFirst();
	}
	else Began = true;

	if(!FieldNames.Empty())
		Me->FieldAssignmentBeginning();

	Me->operator()(t);

	if(!FieldNames.Empty() && Me->Lang.AddFieldNameAfterAssignment)
		Me->FieldAssignmentEnding();

	return *this;
}

template<typename I> forceinline Requires<
	CAsForwardRange<I>::_,
GenericTextDeserializer<TRemoveConstRef<I>>> TextDeserializer(const LanguageParams& language, I&& input)
{return {language, ForwardAsRange<I>(input)};}

INTRA_WARNING_POP

}}
