#pragma once

#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Meta/EachField.h"
#include "Range/StringView.h"
#include "Algo/Search.h"
#include "IO/StaticStream.h"
#include "Data/Reflection.h"
#include "Data/TextSerializationParams.h"
#include "Containers/String.h"
#include "Range/Construction/TakeUntil.h"
#include "Range/Construction/TakeUntilAny.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename O> class GenericTextSerializer
{
public:
	GenericTextSerializer(const DataLanguageParams& langParams, const TextSerializerParams& serializerParams, const O& output):
		Params(serializerParams), Lang(langParams),
		NestingLevel(0), Output(output) {}

	void NextField(TextSerializerParams::TypeFlags typeFlag);

	void EndField(TextSerializerParams::TypeFlags typeFlag);

	void FieldAssignmentBeginning(StringView name);
	void FieldAssignmentEnding(StringView name);

	StringView GetString() const {return Output.GetString();}

	void StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type=TextSerializerParams::TypeFlags_Struct);
	void StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type=TextSerializerParams::TypeFlags_Struct);

	

	//! Сериализовать любой диапазон как массив
	template<typename R> void SerializeRange(const R& range);

	//! Сериализовать булевое значение
	void SerializeBool(bool v);


	//! Сериализовать значение
	template<typename T> forceinline void operator()(const T& v) {SerializeText(*this, v);}

	//! Вычислить размер объекта в сериализованном виде в байтах
	template<typename T> size_t SerializedSizeOf(const T& v) const
	{
		GenericTextSerializer<IO::DummyOutput> dummy(Lang, Params, IO::DummyOutput());
		dummy.NestingLevel = NestingLevel;
		SerializeText(dummy, v);
		return dummy.Output.BytesWritten();
	}

	void ResetOutput(const O& output) {Output=output; NestingLevel=0;}

	TextSerializerParams Params;
	DataLanguageParams Lang;
	int NestingLevel;
	O Output;
};

typedef GenericTextSerializer<IO::MemoryOutput> TextSerializer;
typedef GenericTextSerializer<IO::DummyOutput> DummyTextSerializer;


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
			if(!Log().EndsWith(eofStr)) Log += eofStr;
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
	template<typename R> void DeserializeRange(R& outputRange);

	//! Сериализовать булевое значение
	bool DeserializeBool();

	//! Десериализовать любое значение
	template<typename T> forceinline void operator()(T& value) {DeserializeText(*this, value);}

	//! Десериализовать любое значение
	template<typename T> forceinline T Deserialize()
	{
		T result;
		DeserializeText(*this, result);
		return result;
	}

	void ResetStream(IO::MemoryInput stream) {Input=stream; Line=0;}

	DataLanguageParams Lang;
	int NestingLevel;
	size_t Line;
	IO::MemoryInput Input;
	String Log;
};


template<typename O> struct GenericTextSerializerStructVisitor
{
	GenericTextSerializer<O>* Me;
	bool Began;
	ArrayRange<const StringView> FieldNames;
	TextSerializerParams::TypeFlags Type;

	template<typename T> GenericTextSerializerStructVisitor<O>& operator()(const T& t)
	{
		if(Began)
		{
			Me->NextField(Type);
			if(!FieldNames.Empty()) FieldNames.PopFirst();
		}
		else Began = true;

		if(!FieldNames.Empty())
			Me->FieldAssignmentBeginning(FieldNames.First());

		Me->operator()(t);

		if(!FieldNames.Empty() && Me->Lang.AddFieldNameAfterAssignment)
			Me->FieldAssignmentEnding(FieldNames.First());

		return *this;
	}
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




//! Сериализовать кортеж
template<typename O, typename Tuple> Meta::EnableIf<
	Meta::HasForEachField<Tuple, GenericTextSerializerStructVisitor<O>>::_ &&
	!HasReflection<Tuple>::_
> SerializeText(GenericTextSerializer<O>& serializer,
	const Tuple& src, ArrayRange<const StringView> fieldNames=null)
{
	serializer.StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Tuple);
	if((serializer.Params.ValuePerLine & TextSerializerParams::TypeFlags_Tuple) == 0)
		fieldNames = null;
	GenericTextSerializerStructVisitor<O> visitor = {&serializer, false, fieldNames, TextSerializerParams::TypeFlags_Tuple};
	Meta::ForEachField(src, visitor);
	serializer.StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Tuple);
}

//! Сериализовать структуру или класс со статической рефлексией
template<typename T, typename O> forceinline Meta::EnableIf<
	HasReflection<T>::_
> SerializeText(GenericTextSerializer<O>& serializer, const T& src)
{
	auto fieldNames = T::ReflectionFieldNames();
	serializer.StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Struct);
	if(!serializer.Params.FieldAssignments && !serializer.Lang.RequireFieldAssignments)
		fieldNames=null;
	GenericTextSerializerStructVisitor<O> visitor = {&serializer, false, fieldNames, TextSerializerParams::TypeFlags_Struct};
	Meta::ForEachField(src, visitor);
	serializer.StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Struct);
}



//! Сериализация целых чисел
template<typename T, typename O> Meta::EnableIf<
	Meta::IsIntegralType<T>::_
> SerializeText(GenericTextSerializer<O>& serializer, T v)
{serializer.Output.WriteIntegerText(v);}

//! Сериализация чисел с плавающей запятой
template<typename T, typename O> Meta::EnableIf<
	Meta::IsFloatType<T>::_
> SerializeText(GenericTextSerializer<O>& serializer, const T& v)
{serializer.Output.WriteFloatText(v, sizeof(T)<=4? 7: 15, serializer.Lang.DecimalSeparator);}


//! Сериализовать строку
template<typename O, typename Char, typename Allocator> forceinline void SerializeText(
	GenericTextSerializer<O>& serializer, const GenericString<Char, Allocator>& v)
{SerializeText(serializer, GenericStringView<Char>(v));}

//! Сериализовать булевое значение
template<typename O> forceinline void SerializeText(
	GenericTextSerializer<O>& serializer, bool v)
{serializer.SerializeBool(v);}

//! Сериализовать диапазон
template<typename R, typename O> forceinline Meta::EnableIf<
	Range::IsInputRange<R>::_
> SerializeText(GenericTextSerializer<O>& serializer, const R& range)
{serializer.SerializeRange(range);}

//! Десериализовать диапазон
template<typename R> forceinline Meta::EnableIf<
	Range::IsOutputRange<R>::_
> DeserializeText(TextDeserializer& deserializer, R& range)
{deserializer.DeserializeRange(range);}

//! Десериализовать массив
template<typename T, size_t N> forceinline void DeserializeText(
	TextDeserializer& deserializer, T(&arr)[N])
{
	ArrayRange<T> range = arr;
	deserializer.DeserializeRange(range);
}

#if INTRA_DISABLED
//! Сериализовать список
template<typename T, typename O> forceinline void SerializeText(
	GenericTextSerializer<O>& serializer, const List<T>& list)
{serializer.SerializeRange(list.AsRange());}
#endif

//! Сериализовать массив
template<typename T, typename O> forceinline void SerializeText(
	GenericTextSerializer<O>& serializer, const Array<T>& arr)
{serializer.SerializeRange(arr.AsRange());}

//! Десериализовать массив
template<typename T> forceinline void DeserializeText(TextDeserializer& deserializer, Array<T>& arr)
{
	arr.SetCount(0);
	auto inserter = arr.Insert($);
	deserializer.DeserializeRange(inserter);
}

//! Сериализовать массив фиксированной длины
template<typename T, size_t N, typename O> forceinline void SerializeText(
	GenericTextSerializer<O>& serializer, const T(&src)[N])
{serializer.SerializeRange(AsRange(src));}



//! Десериализация вещественных чисел
template<typename T> forceinline Meta::EnableIf<
	Meta::IsFloatType<T>::_
> DeserializeText(TextDeserializer& deserializer, T& v)
{v = deserializer.Input.Parse<T>(deserializer.Lang.DecimalSeparator);}

//! Десериализация целых чисел
template<typename T> forceinline Meta::EnableIf<
	Meta::IsIntegralType<T>::_
> DeserializeText(TextDeserializer& deserializer, T& v)
{v = deserializer.Input.Parse<T>();}

//! Десериализация bool
forceinline void DeserializeText(TextDeserializer& deserializer, bool& v)
{v = deserializer.DeserializeBool();}


//! Десериализация структур
template<typename T> forceinline Meta::EnableIf<
	HasReflection<T>::_
> DeserializeText(TextDeserializer& deserializer, T& v)
{deserializer.DeserializeStruct(v);}

//! Десериализация кортежей
template<typename T> forceinline Meta::EnableIf<
	Meta::IsTuple<T>::_
> DeserializeText(TextDeserializer& deserializer, T& v)
{deserializer.DeserializeTuple(v);}

INTRA_WARNING_POP

}}

#include "TextSerialization.inl"
