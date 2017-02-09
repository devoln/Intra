#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
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
#include "Range/Decorators/TakeUntil.h"
#include "Range/Decorators/TakeUntilAny.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename O> class GenericTextSerializer;

template<typename O> struct GenericTextSerializerStructVisitor
{
	GenericTextSerializer<O>* Me;
	bool Began;
	ArrayRange<const StringView> FieldNames;
	TextSerializerParams::TypeFlags Type;

	template<typename T> GenericTextSerializerStructVisitor<O>& operator()(T&& t);
};

template<typename O> class GenericTextSerializer
{
public:
	GenericTextSerializer(const DataLanguageParams& langParams,
		const TextSerializerParams& serializerParams, const O& output):
		Params(serializerParams), Lang(langParams),
		NestingLevel(0), Output(output) {}

	void NextField(TextSerializerParams::TypeFlags typeFlag);

	void EndField(TextSerializerParams::TypeFlags typeFlag);

	void FieldAssignmentBeginning(StringView name);
	void FieldAssignmentEnding(StringView name);

	StringView GetString() const {return Output.GetString();}

	void StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type=TextSerializerParams::TypeFlags_Struct);
	void StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type=TextSerializerParams::TypeFlags_Struct);

	//! Сериализовать кортеж
	template<typename Tuple> Meta::EnableIf<
		Meta::HasForEachField<Tuple, GenericTextSerializerStructVisitor<O>>::_ &&
		!HasReflectionFieldNamesMethod<Tuple>::_
	> SerializeTuple(Tuple&& src, ArrayRange<const StringView> fieldNames = null)
	{
		StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Tuple);
		if((Params.ValuePerLine & TextSerializerParams::TypeFlags_Tuple) == 0)
			fieldNames = null;
		GenericTextSerializerStructVisitor<O> visitor = {this,
			false, fieldNames, TextSerializerParams::TypeFlags_Tuple};
		Meta::ForEachField(src, visitor);
		StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Tuple);
	}


	//! Сериализатор как функтор
	template<typename T> forceinline GenericTextSerializer& operator()(T&& v) {return *this << Meta::Forward<T>(v);}

	//! Вычислить размер объекта в сериализованном виде в байтах
	template<typename T> size_t SerializedSizeOf(T&& v) const
	{
		GenericTextSerializer<IO::DummyOutput> dummy(Lang, Params, IO::DummyOutput());
		dummy.NestingLevel = NestingLevel;
		dummy << Meta::Forward<T>(v);
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

template<typename O> template<typename T> GenericTextSerializerStructVisitor<O>&
	GenericTextSerializerStructVisitor<O>::operator()(T&& t)
{
	if(Began)
	{
		Me->NextField(Type);
		if(!FieldNames.Empty()) FieldNames.PopFirst();
	}
	else Began = true;

	if(!FieldNames.Empty())
		Me->FieldAssignmentBeginning(FieldNames.First());

	*Me << Meta::Forward<T>(t);

	if(!FieldNames.Empty() && Me->Lang.AddFieldNameAfterAssignment)
		Me->FieldAssignmentEnding(FieldNames.First());

	return *this;
}

//! Сериализовать структуру или класс со статической рефлексией
template<typename T, typename O> forceinline Meta::EnableIf<
	Meta::HasForEachField<T>::_ &&
	HasReflectionFieldNamesMethod<T>::_,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, T&& src)
{
	auto fieldNames = Meta::RemoveConstRef<T>::ReflectionFieldNames();
	serializer.StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Struct);
	if(!serializer.Params.FieldAssignments && !serializer.Lang.RequireFieldAssignments)
		fieldNames = null;
	GenericTextSerializerStructVisitor<O> visitor = {&serializer,
		false, fieldNames, TextSerializerParams::TypeFlags_Struct};
	Meta::ForEachField(Meta::Forward<T>(src), visitor);
	serializer.StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Struct);
	return serializer;
}

//! Сериализовать кортеж
template<typename O, typename Tuple> Meta::EnableIf<
	Meta::HasForEachField<Tuple, GenericTextSerializerStructVisitor<O>>::_ &&
	!HasReflectionFieldNamesMethod<Tuple>::_,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, Tuple&& src)
{
	serializer.SerializeTuple(Meta::Forward<Tuple>(src));
	return serializer;
}


//! Сериализация целых чисел
template<typename T, typename O> Meta::EnableIf<
	Meta::IsIntegralType<T>::_,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, T v)
{
	serializer.Output.WriteIntegerText(v);
	return serializer;
}

//! Сериализация чисел с плавающей запятой
template<typename T, typename O> Meta::EnableIf<
	Meta::IsFloatType<T>::_,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, T v)
{
	serializer.Output.WriteFloatText(v, sizeof(T)<=4? 7: 15, serializer.Lang.DecimalSeparator);
	return serializer;
}

//! Сериализовать булевое значение
template<typename O> forceinline GenericTextSerializer<O>& operator<<(
	GenericTextSerializer<O>& serializer, bool v)
{
	INTRA_ASSERT(int(v) <= 1 && "Invalid bool value!");
	serializer.Output.WriteShortRaw(serializer.Lang.FalseTrueNames[v!=false]);
	return serializer;
}

//! Сериализовать диапазон
template<typename R, typename O> forceinline Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ &&
	!Range::IsAsCharRange<R>::_,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, R&& r)
{
	serializer.Output.WriteShortRaw(serializer.Lang.ArrayOpen);
	auto range = Range::Forward<R>(r);
	if(!range.Empty())
	{
		typedef Range::ValueTypeOfAs<R> T;
		
		bool TypeIsSimpleArray = Meta::IsArithmeticType<T>::_ ||
			Meta::IsPointerType<T>::_ || Range::IsAsCharRange<T>::_;

		const int nestLevelUp = int(!TypeIsSimpleArray &&
			serializer.Lang.ArrayOpen!=null && serializer.Lang.ArrayClose!=null);
		serializer.NestingLevel += nestLevelUp;
		if(!TypeIsSimpleArray && !serializer.Lang.StructInstanceOpen.Empty())
			serializer.EndField(TextSerializerParams::TypeFlags_StructArray);

		serializer << range.First();
		range.PopFirst();
		while(!range.Empty())
		{
			serializer.NextField(TypeIsSimpleArray?
				TextSerializerParams::TypeFlags_Array:
				TextSerializerParams::TypeFlags_StructArray);
			serializer << range.First();
			range.PopFirst();
		}

		serializer.NestingLevel -= nestLevelUp;
		if(!TypeIsSimpleArray && !serializer.Lang.StructInstanceClose.Empty())
			serializer.EndField(TextSerializerParams::TypeFlags_StructArray);
	}
	serializer.Output.WriteShortRaw(serializer.Lang.ArrayClose);
	return serializer;
}

template<typename O> GenericTextSerializer<O>& operator<<(GenericTextSerializer<O>& serializer, const StringView& v)
{
	serializer.Output.WriteShortRaw(serializer.Lang.StringQuote);
	serializer.Output.WriteReplacedString(v, {"\n", "\r", "\t"}, {"\\n", "\\r", "\\t"});
	serializer.Output.WriteShortRaw(serializer.Lang.StringQuote);
	return serializer;
}

//! Сериализовать массив фиксированной длины или строковой литерал
template<typename T, size_t N, typename O> forceinline GenericTextSerializer<O>& operator<<(
	GenericTextSerializer<O>& serializer, const T(&src)[N])
{return serializer << Range::AsRange(src);}


template<typename O> void GenericTextSerializer<O>::NextField(TextSerializerParams::TypeFlags typeFlag)
{
	if(typeFlag & TextSerializerParams::TypeFlags_Struct)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Struct);
		Output.WriteShortRaw(Lang.FieldSeparator);
	}
	if(typeFlag & TextSerializerParams::TypeFlags_Tuple)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Tuple);
		Output.WriteShortRaw(Lang.TupleFieldSeparator);
	}
	if(typeFlag & TextSerializerParams::TypeFlags_Array)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Array);
		Output.WriteShortRaw(Lang.ArrayElementSeparator);
	}
	if(typeFlag & TextSerializerParams::TypeFlags_StructArray)
	{
		INTRA_ASSERT(typeFlag==TextSerializerParams::TypeFlags_StructArray);
		Output.WriteShortRaw(Lang.ArrayElementSeparator);
	}
	EndField(typeFlag);
}

template<typename O> void GenericTextSerializer<O>::EndField(TextSerializerParams::TypeFlags typeFlag)
{
	if((Params.ValuePerLine & typeFlag) == 0)
	{
		Output.WriteCharRaw(' ');
		return;
	}
	Output.WriteShortRaw(Params.LineEnding);
	if((Params.UseTabs & typeFlag)==0) return;
	for(int i=0; i<NestingLevel; i++)
		Output.WriteShortRaw(Params.TabChars);
}

template<typename O> void GenericTextSerializer<O>::FieldAssignmentBeginning(StringView name)
{
	INTRA_ASSERT(name!=null);
	Output.WriteShortRaw(Lang.LeftFieldNameBeginQuote);
	Output.WriteRaw(name);
	Output.WriteShortRaw(Lang.LeftFieldNameEndQuote);

	if(Params.UseAssignmentSpaces)
		Output.WriteCharRaw(' ');

	Output.WriteShortRaw(Lang.LeftAssignmentOperator);

	if(Params.UseAssignmentSpaces && Lang.LeftAssignmentOperator!=null)
		Output.WriteCharRaw(' ');
}

template<typename O> void GenericTextSerializer<O>::FieldAssignmentEnding(StringView name)
{
	INTRA_ASSERT(name!=null);

	if(Params.UseAssignmentSpaces)
		Output.WriteCharRaw(' ');

	Output.WriteShortRaw(Lang.RightAssignmentOperator);

	if(Params.UseAssignmentSpaces && Lang.RightAssignmentOperator!=null)
		Output.WriteCharRaw(' ');

	Output.WriteShortRaw(Lang.RightFieldNameBeginQuote);
	Output.WriteRaw(name);
	Output.WriteShortRaw(Lang.RightFieldNameEndQuote);
}

template<typename O> void GenericTextSerializer<O>::StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type)
{
	Output.WriteShortRaw((type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceOpen: Lang.TupleOpen);
	NestingLevel++;
	EndField(type);
}

template<typename O> void GenericTextSerializer<O>::StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type)
{
	NestingLevel--;
	EndField(type);
	Output.WriteShortRaw((type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceClose: Lang.TupleClose);
}

INTRA_WARNING_POP

}}
