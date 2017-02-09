#pragma once

#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Meta/EachField.h"
#include "Range/Generators/StringView.h"
#include "Algo/Search.hh"
#include "Algo/Comparison/EndsWith.h"
#include "IO/StaticStream.h"
#include "Data/Reflection.h"
#include "TextSerializerParams.h"
#include "Containers/String.h"
#include "Containers/Operations.h"
#include "Range/Decorators/TakeUntil.h"
#include "Range/Decorators/TakeUntilAny.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class TextDeserializer
{
public:
	TextDeserializer(const DataLanguageParams& langParams, const IO::MemoryInput& input):
		Lang(langParams), NestingLevel(0), Line(0), Input(input), Log() {}


	bool IgnoreField();
	bool IgnoreArrayElement();
	bool NextField(TextSerializerParams::TypeFlags typeFlag);

	//! Прочитать имя поля и оператор присваивания, перейдя к правой части присваивания.
	//! Если в процессе обнаруживается ошибка, функция возвращает на исходную позицию и возвращает null
	StringView FieldAssignmentBeginning();
	StringView FieldAssignmentEnding();

	void StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type);
	void StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type);

	forceinline void SkipAllSpaces()
	{
		Line += Input.SkipAllSpaces();
	}

	void SkipAllSpacesAndComments()
	{
		for(;;)
		{
			SkipAllSpaces();
			if(!Lang.OneLineCommentBegin.Empty() && Input.Expect(Lang.OneLineCommentBegin))
			{
				int counter = 1;
				StringView commentBlock = Input.ReadRecursiveBlock(counter,
					Lang.OneLineCommentBegin, "\n", null, null);
				Line += Algo::CountLinesAdvance(commentBlock);
				continue;
			}
			if(!Lang.MultiLineCommentBegin.Empty() && Input.Expect(Lang.MultiLineCommentBegin))
			{
				int counter = 1;
				StringView commentBlock = Input.ReadRecursiveBlock(counter,
					Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd, null, null);
				Line += Algo::CountLinesAdvance(commentBlock);
				continue;
			}
			break;
		}
	}
	
	bool Expect(StringView stringToExpect)
	{
		SkipAllSpacesAndComments();
		if(Input.Expect(stringToExpect)) return true;
		if(Input.Rest==null)
		{
			static const StringView eofStr = "Unexpected EOF!\r\n";
			if(!Algo::EndsWith(Log, eofStr)) Log += eofStr;
			return false;
		}
		LogExpectError(Range::TakeUntil(Input.Rest, AsciiSet::Spaces), {stringToExpect});
		return false;
	}

	intptr ExpectOneOf(ArrayRange<const StringView> stringsToExpect)
	{
		SkipAllSpaces();
		size_t maxStrLength=0;
		for(size_t i=0; i<stringsToExpect.Length(); i++)
		{
			if(Input.Expect(stringsToExpect[i])) return intptr(i);
			if(maxStrLength<stringsToExpect[i].Length())
				maxStrLength = stringsToExpect[i].Length();
		}
		LogExpectError(Range::TakeUntil(Input.Rest, AsciiSet::Spaces), stringsToExpect);
		return -1;
	}

	void LogExpectError(StringView token, ArrayRange<const StringView> expected)
	{
		Log += String::Format()("Error(")(Line)(":")(Input.Position())("): token \"")(token)("\", where expected ");
		if(expected.Length()>1) Log += "one of the folowing tokens: " + ToString(expected, ", ", "\"", "\"");
		if(expected.Length()==1) Log += "token \"" + expected[0] + "\"";
		Log += ".\r\n";
	}


	//! Десериализовать структуру со статической информацией о полях
	template<typename T> void DeserializeStruct(T& dst, ArrayRange<const StringView> fieldNames);

	//! Десериализовать структуру со статической информацией о полях
	template<typename T> void DeserializeStruct(T& dst) {DeserializeStruct(dst, T::ReflectionFieldNames());}

	//! Десериализовать структуру со статической информацией о полях
	template<typename T> void DeserializeTuple(T& dst);

	//! Десериализовать любой контейнер с одним шаблонным параметром как массив
	template<typename R> void DeserializeRange(R&& outputRange);

	//! Десериализовать любое значение
	template<typename T> forceinline Meta::EnableIf<
		!Meta::IsConst<T>::_,
	TextDeserializer&> operator()(T&& value) {return *this >> Meta::Forward<T>(value);}

	//! Десериализовать любое значение
	template<typename T> forceinline T Deserialize()
	{
		T result;
		*this >> result;
		return result;
	}

	void ResetStream(IO::MemoryInput stream) {Input=stream; Line=0;}

	DataLanguageParams Lang;
	int NestingLevel;
	size_t Line;
	IO::MemoryInput Input;
	String Log;
};

struct TextDeserializerStructVisitor
{
	TextDeserializer* Me;
	ArrayRange<const StringView> FieldNames;
	bool Began;
	TextSerializerParams::TypeFlags Type;

	template<typename T> TextDeserializerStructVisitor& operator()(T& t)
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
};


//! Десериализация вещественных чисел
template<typename T> forceinline Meta::EnableIf<
	Meta::IsFloatType<T>::_,
TextDeserializer&> operator>>(TextDeserializer& deserializer, T& v)
{
	v = deserializer.Input.Parse<T>(deserializer.Lang.DecimalSeparator);
	return deserializer;
}

//! Десериализация целых чисел
template<typename T> forceinline Meta::EnableIf<
	Meta::IsIntegralType<T>::_,
TextDeserializer&> operator>>(TextDeserializer& deserializer, T& v)
{
	v = deserializer.Input.Parse<T>();
	return deserializer;
}

//! Десериализация bool
forceinline TextDeserializer& operator>>(TextDeserializer& deserializer, bool& v)
{
	deserializer.SkipAllSpaces();
	StringView identifier = deserializer.Input.ParseIdentifier();
	if(identifier==deserializer.Lang.FalseTrueNames[true])
	{
		v = true;
		return deserializer;
	}
	if(identifier!=deserializer.Lang.FalseTrueNames[false])
		deserializer.LogExpectError(identifier, deserializer.Lang.FalseTrueNames);
	v = false;
	return deserializer;
}


//! Десериализация структур
template<typename T> forceinline Meta::EnableIf<
	HasReflectionFieldNamesMethod<T>::_,
TextDeserializer&> operator>>(TextDeserializer& deserializer, T& v)
{
	deserializer.DeserializeStruct(v);
	return deserializer;
}

//! Десериализация кортежей
template<typename T> forceinline Meta::EnableIf<
	Meta::IsTuple<T>::_,
TextDeserializer&> operator>>(TextDeserializer& deserializer, T& v)
{
	deserializer.DeserializeTuple(v);
	return deserializer;
}

inline bool TextDeserializer::IgnoreArrayElement()
{
	int counter=1;
	Input.ReadRecursiveBlock(counter, null, Lang.ArrayClose, Lang.ArrayElementSeparator,
	{
		{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
		{Lang.OneLineCommentBegin, StringView("\n")},
		{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd}
	},
	{
		{Lang.StructInstanceOpen, Lang.StructInstanceClose},
		{Lang.ArrayOpen, Lang.ArrayClose}
	});
	if(counter==0)
	{
		Input.Rest = StringView(Input.Rest.Data()-Lang.ArrayClose.Length(), Input.Rest.End());
		return true;
	}
	INTRA_ASSERT(counter==1);
	return false;
}

inline bool TextDeserializer::IgnoreField()
{
	int counter=1;
	Input.ReadRecursiveBlock(counter, Lang.StructInstanceOpen, Lang.StructInstanceClose, Lang.FieldSeparator,
	{
		{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
		{Lang.OneLineCommentBegin, "\n"},
		{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
		{Lang.ArrayOpen, Lang.ArrayClose}
	});
	INTRA_ASSERT(counter==0 || counter==1);
	return counter==0;
}

inline bool TextDeserializer::NextField(TextSerializerParams::TypeFlags typeFlag)
{
	if(typeFlag & TextSerializerParams::TypeFlags_Struct)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Struct);
		if(!Expect(Lang.FieldSeparator))
		{
			if(!Input.Rest.Empty()) IgnoreField();
			return false;
		}
	}
	if(typeFlag & TextSerializerParams::TypeFlags_Tuple)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Tuple);
		if(!Expect(Lang.TupleFieldSeparator))
		{
			if(!Input.Rest.Empty()) IgnoreField();
			return false;
		}
	}
	if(typeFlag & TextSerializerParams::TypeFlags_Array)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Array);
		if(!Expect(Lang.ArrayElementSeparator))
			return !Input.Rest.Empty() && !IgnoreArrayElement();
	}
	if(typeFlag & TextSerializerParams::TypeFlags_StructArray)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_StructArray);
		if(!Expect(Lang.ArrayElementSeparator))
			return !Input.Rest.Empty() && !IgnoreArrayElement();
	}
	SkipAllSpacesAndComments();
	return true;
}

inline StringView TextDeserializer::FieldAssignmentBeginning()
{
	auto oldPos = Input.Rest;
	auto oldLine = Line;
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.LeftFieldNameBeginQuote))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	SkipAllSpacesAndComments();
	StringView name = Input.ParseIdentifier();
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.LeftFieldNameEndQuote))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.LeftAssignmentOperator))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	return name;
}

inline StringView TextDeserializer::FieldAssignmentEnding()
{
	auto oldPos = Input.Rest;
	auto oldLine = Line;
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.RightAssignmentOperator))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.RightFieldNameBeginQuote))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	SkipAllSpacesAndComments();
	StringView name = Input.ParseIdentifier();
	SkipAllSpacesAndComments();
	if(!Input.Expect(Lang.RightFieldNameEndQuote))
	{
		Input.Rest = oldPos;
		Line = oldLine;
		return null;
	}
	return name;
}

inline void TextDeserializer::StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type)
{
	if(!Expect((type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceOpen: Lang.TupleOpen))
	{
		if(Input.Rest.Empty()) return;
		int counter = -1;
		Input.ReadRecursiveBlock(counter, Lang.StructInstanceOpen, null, null,
			{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			{
				{Lang.ArrayOpen, Lang.ArrayClose}
			}
		);
	}
	NestingLevel++;
}

inline void TextDeserializer::StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type)
{
	NestingLevel--;
	if(!Expect((type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceClose: Lang.TupleClose))
	{
		int counter = 1;
		Input.ReadRecursiveBlock(counter, Lang.StructInstanceOpen, Lang.StructInstanceClose, null,
			{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			{
				{Lang.ArrayOpen, Lang.ArrayClose}
			}
		);
	}
}



template<typename T> void TextDeserializer::DeserializeStruct(T& dst, ArrayRange<const StringView> fieldNames)
{
	StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Struct);
	TextDeserializerStructVisitor visitor{this, fieldNames, false, TextSerializerParams::TypeFlags_Struct};
	if(fieldNames.Empty())
	{
		Meta::ForEachField(dst, visitor);
		StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Struct);
		return;
	}

	bool expectSeparator=false;
	for(size_t i=0; ; i++)
	{
		SkipAllSpacesAndComments();
		if(Algo::StartsWith(Input.Rest, Lang.StructInstanceClose)) break;

		if(expectSeparator)
		{
			if(!NextField(TextSerializerParams::TypeFlags_Struct)) break;
		}
		expectSeparator=true;

		StringView name = FieldAssignmentBeginning();

		//TODO: учесть, что имя поля может идти после его значения
		size_t index = 0;
		if(name.Empty()) index = fieldNames.Length();
		else index = Algo::CountUntil(fieldNames, name);
		if(index==fieldNames.Length() && name!=null) //Если такого поля в структуре нет, пропускаем его
		{
			const bool finished = IgnoreField();
			if(!finished)
			{
				expectSeparator=false;
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

template<typename T> void TextDeserializer::DeserializeTuple(T& dst)
{
	StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Tuple);
	TextDeserializerStructVisitor visitor{this, null, false, TextSerializerParams::TypeFlags_Tuple};
	Meta::ForEachField(dst, visitor);
	StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Tuple);
}

inline TextDeserializer& operator>>(TextDeserializer& deserializer, String& dst)
{
	if(!deserializer.Expect(deserializer.Lang.StringQuote))
		return deserializer;

	StringView escapedResult = deserializer.Input.ReadUntil(deserializer.Lang.StringQuote);
	deserializer.Input.Rest.PopFirstN(deserializer.Lang.StringQuote.Length());

	dst.SetLengthUninitialized(Algo::StringMultiReplaceAsciiLength(escapedResult, {"\\n", "\\r", "\\t"}, {"\n", "\r", "\t"}));
	auto dstRange = dst.AsRange();
	Algo::StringMultiReplaceAscii(escapedResult, dstRange, {"\\n", "\\r", "\\t"}, {"\n", "\r", "\t"});


	/*Range::CountRange<char> counter;
	const KeyValuePair<StringView, StringView> replacements[] = {{"\\n", "\n"}, {"\\r", "\r"}, {"\\t", "\t"}};
	escapedResult.MultiReplaceToAdvance(counter, AsRange(replacements));
	dst.SetLengthUninitialized(counter.Counter);
	auto strData = dst.AsRange();
	escapedResult.MultiReplaceToAdvance(strData, AsRange(replacements));*/

	return deserializer;
}



//! Десериализовать диапазон
template<typename R> forceinline Meta::EnableIf<
	Range::IsOutputRange<R>::_,
TextDeserializer&> operator>>(TextDeserializer& deserializer, R&& range)
{
	deserializer.DeserializeRange(range);
	return deserializer;
}

//! Десериализовать массив
template<typename T, size_t N> forceinline TextDeserializer& operator>>(
	TextDeserializer& deserializer, T(&arr)[N])
{
	deserializer.DeserializeRange(Range::AsRange(arr));
	return deserializer;
}

static_assert(Container::IsClearable<Array<String>>::_ &&
	Container::IsLastAppendable<Array<String>>::_, "ERROR!");

//! Десериализовать массив
template<typename C> forceinline Meta::EnableIf<
	Container::IsClearable<C>::_ &&
	Container::IsLastAppendable<C>::_,
TextDeserializer&> operator>>(TextDeserializer& deserializer, C& container)
{
	Container::SetCount0(container);
	deserializer.DeserializeRange(Range::LastAppender(container));
	return deserializer;
}



template<typename R> void TextDeserializer::DeserializeRange(R&& outputRange)
{
	typedef Range::ValueTypeOf<R> T;

	if(!Expect(Lang.ArrayOpen))
	{
		int counter = -1;
		Input.ReadRecursiveBlock(counter, Lang.ArrayOpen, null, null,
			{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			{
				{Lang.StructInstanceOpen, Lang.StructInstanceClose}
			}
		);
	}

	SkipAllSpacesAndComments();
	if( (Lang.ArrayClose.Empty() && !Algo::StartsWith(Input.Rest, Lang.RightFieldNameBeginQuote)) ||
		!Algo::StartsWith(Input.Rest, Lang.ArrayClose) )
	{
		outputRange.Put(Deserialize<T>());
		SkipAllSpacesAndComments();
		while( (Lang.ArrayClose.Empty() && !Algo::StartsWith(Input.Rest, Lang.RightFieldNameBeginQuote)) ||
			!Algo::StartsWith(Input.Rest, Lang.ArrayClose) )
		{
			if(NextField(TextSerializerParams::TypeFlags_Array))
				outputRange.Put(Deserialize<T>());
			else if(Input.Rest.Empty()) return;
			SkipAllSpacesAndComments();
		}
	}

	if(!Expect(Lang.ArrayClose))
	{
		int counter = 1;
		Input.ReadRecursiveBlock(counter, Lang.ArrayOpen, Lang.ArrayClose, null,
			{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			{
				{Lang.StructInstanceOpen, Lang.StructInstanceClose}
			}
		);
	}
}

INTRA_WARNING_POP

}}
