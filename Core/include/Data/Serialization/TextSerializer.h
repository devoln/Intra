#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Meta/EachField.h"
#include "Range/Generators/StringView.h"
#include "Algo/Search.hh"
#include "Algo/Comparison/EndsWith.h"
#include "Algo/String/ToStringArithmetic.h"
#include "Data/Reflection.h"
#include "TextSerializerParams.h"
#include "LanguageParams.h"
#include "Range/Decorators/TakeUntil.h"
#include "Range/Decorators/TakeUntilAny.h"
#include "Range/Output/OutputArrayRange.h"
#include "Range/Compositors/ZipKV.h"
#include "Algo/Mutation/ReplaceSubrange.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename O> class GenericTextSerializer;

template<typename O> struct GenericTextSerializerStructVisitor
{
	GenericTextSerializer<O>* Me;
	bool Began;
	CSpan<StringView> FieldNames;
	TextSerializerParams::TypeFlags Type;

	template<typename T> GenericTextSerializerStructVisitor<O>& operator()(T&& t);
};

template<typename O> class GenericTextSerializer
{
public:
	GenericTextSerializer(const LanguageParams& langParams,
		const TextSerializerParams& serializerParams, const O& output):
		Params(serializerParams), Lang(langParams),
		NestingLevel(0), Output(output) {}

	void NextField(TextSerializerParams::TypeFlags typeFlag)
	{
		if(typeFlag & TextSerializerParams::TypeFlags_Struct)
		{
			INTRA_DEBUG_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Struct);
			Algo::CopyToAdvanceByOne(Lang.FieldSeparator, Output);
		}
		if(typeFlag & TextSerializerParams::TypeFlags_Tuple)
		{
			INTRA_DEBUG_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Tuple);
			Algo::CopyToAdvanceByOne(Lang.TupleFieldSeparator, Output);
		}
		if(typeFlag & TextSerializerParams::TypeFlags_Array)
		{
			INTRA_DEBUG_ASSERT(typeFlag==TextSerializerParams::TypeFlags_Array);
			Algo::CopyToAdvanceByOne(Lang.ArrayElementSeparator, Output);
		}
		if(typeFlag & TextSerializerParams::TypeFlags_StructArray)
		{
			INTRA_DEBUG_ASSERT(typeFlag==TextSerializerParams::TypeFlags_StructArray);
			Algo::CopyToAdvanceByOne(Lang.ArrayElementSeparator, Output);
		}
		EndField(typeFlag);
	}

	void EndField(TextSerializerParams::TypeFlags typeFlag)
	{
		if((Params.ValuePerLine & typeFlag) == 0)
		{
			Output.Put(' ');
			return;
		}
		Algo::CopyToAdvanceByOne(Params.LineEnding, Output);
		if((Params.UseTabs & typeFlag)==0) return;
		for(int i=0; i<NestingLevel; i++)
			Algo::CopyToAdvanceByOne(Params.TabChars, Output);
	}

	void FieldAssignmentBeginning(StringView name)
	{
		INTRA_DEBUG_ASSERT(!name.Empty());
		Algo::CopyToAdvanceByOne(Lang.LeftFieldNameBeginQuote, Output);
		Algo::CopyToAdvance(name, Output);
		Algo::CopyToAdvanceByOne(Lang.LeftFieldNameEndQuote, Output);

		if(Params.UseAssignmentSpaces)
			Output.Put(' ');

		Algo::CopyToAdvanceByOne(Lang.LeftAssignmentOperator, Output);

		if(Params.UseAssignmentSpaces && Lang.LeftAssignmentOperator!=null)
			Output.Put(' ');
	}

	void FieldAssignmentEnding(StringView name)
	{
		INTRA_DEBUG_ASSERT(name!=null);

		if(Params.UseAssignmentSpaces)
			Output.Put(' ');

		Algo::CopyToAdvanceByOne(Lang.RightAssignmentOperator, Output);

		if(Params.UseAssignmentSpaces && Lang.RightAssignmentOperator!=null)
			Output.Put(' ');

		Algo::CopyToAdvanceByOne(Lang.RightFieldNameBeginQuote, Output);
		Algo::CopyToAdvance(name, Output);
		Algo::CopyToAdvanceByOne(Lang.RightFieldNameEndQuote, Output);
	}

	void StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type=TextSerializerParams::TypeFlags_Struct)
	{
		auto openStr = (type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceOpen: Lang.TupleOpen;
		Algo::CopyToAdvanceByOne(openStr, Output);
		NestingLevel++;
		EndField(type);
	}

	void StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type=TextSerializerParams::TypeFlags_Struct)
	{
		NestingLevel--;
		EndField(type);
		auto closeStr = (type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceClose: Lang.TupleClose;
		Algo::CopyToAdvanceByOne(closeStr, Output);
	}

	//! Сериализовать кортеж
	template<typename Tuple> Meta::EnableIf<
		Meta::HasForEachField<Tuple, GenericTextSerializerStructVisitor<O>>::_ &&
		!HasReflectionFieldNamesMethod<Tuple>::_
	> SerializeTuple(Tuple&& src, CSpan<StringView> fieldNames = null)
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
		GenericTextSerializer<Range::CountRange<char>> dummy(Lang, Params, Range::CountRange<char>());
		dummy.NestingLevel = NestingLevel;
		dummy << Meta::Forward<T>(v);
		return dummy.Output.Counter;
	}

	void ResetOutput(const O& output) {Output=output; NestingLevel=0;}

	TextSerializerParams Params;
	LanguageParams Lang;
	int NestingLevel;
	O Output;
};

typedef GenericTextSerializer<Range::OutputArrayRange<char>> TextSerializer;
typedef GenericTextSerializer<Range::CountRange<char>> DummyTextSerializer;

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
	serializer.Output << v;
	return serializer;
}

//! Сериализация чисел с плавающей запятой
template<typename T, typename O> Meta::EnableIf<
	Meta::IsFloatType<T>::_,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, T v)
{
	Algo::ToString(serializer.Output, v, sizeof(T)<=4? 7: 15, serializer.Lang.DecimalSeparator);
	return serializer;
}

//! Сериализовать булевое значение
template<typename O> forceinline GenericTextSerializer<O>& operator<<(
	GenericTextSerializer<O>& serializer, bool v)
{
	INTRA_DEBUG_ASSERT(int(v) <= 1 && "Invalid bool value!");
	Algo::CopyToAdvanceByOne(serializer.Lang.FalseTrueNames[v!=false], serializer.Output);
	return serializer;
}

//! Сериализовать диапазон
template<typename R, typename O> forceinline Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ &&
	!Range::IsAsCharRange<R>::_,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, R&& r)
{
	Algo::CopyToAdvanceByOne(serializer.Lang.ArrayOpen, serializer.Output);
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
	Algo::CopyToAdvanceByOne(serializer.Lang.ArrayClose, serializer.Output);
	return serializer;
}

template<typename O> GenericTextSerializer<O>& operator<<(GenericTextSerializer<O>& serializer, const StringView& v)
{
	Algo::CopyToAdvanceByOne(serializer.Lang.StringQuote, serializer.Output);
	Algo::MultiReplaceToAdvance(v, serializer.Output, Range::ZipKV(
		CSpan<StringView>{"\n", "\r", "\t"},
		CSpan<StringView>{"\\n", "\\r", "\\t"}));
	Algo::CopyToAdvanceByOne(serializer.Lang.StringQuote, serializer.Output);
	return serializer;
}

//! Сериализовать массив фиксированной длины или строковой литерал
template<typename T, size_t N, typename O> forceinline GenericTextSerializer<O>& operator<<(
	GenericTextSerializer<O>& serializer, const T(&src)[N])
{return serializer << Range::AsRange(src);}



INTRA_WARNING_POP

}}
