#pragma once

#include "Intra/Type.h"
#include "Intra/Container/Tuple.h"
#include "Intra/EachField.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/Search/Subrange.h"
#include "Intra/Range/Count.h"
#include "Intra/Range/Comparison.h"
#include "Intra/Range/Stream/ToStringArithmetic.h"
#include "Intra/Reflection.h"
#include "TextSerializerParams.h"
#include "LanguageParams.h"
#include "Intra/Range/TakeUntil.h"
#include "Intra/Range/TakeUntilAny.h"

#include "Intra/Range/Zip.h"
#include "Intra/Range/Mutation/ReplaceSubrange.h"

INTRA_BEGIN

template<typename O> class GenericTextSerializer;

template<typename O> struct GenericTextSerializerStructVisitor
{
	GenericTextSerializer<O>* Me;
	bool Began;
	CSpan<const char*> FieldNames;
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
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags_Struct);
			CopyToAdvanceByOne(Lang.FieldSeparator, Output);
		}
		if(typeFlag & TextSerializerParams::TypeFlags_Tuple)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags_Tuple);
			CopyToAdvanceByOne(Lang.TupleFieldSeparator, Output);
		}
		if(typeFlag & TextSerializerParams::TypeFlags_Array)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags_Array);
			CopyToAdvanceByOne(Lang.ArrayElementSeparator, Output);
		}
		if(typeFlag & TextSerializerParams::TypeFlags_StructArray)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags_StructArray);
			CopyToAdvanceByOne(Lang.ArrayElementSeparator, Output);
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
		CopyToAdvanceByOne(Params.LineEnding, Output);
		if((Params.UseTabs & typeFlag)==0) return;
		for(int i=0; i<NestingLevel; i++)
			CopyToAdvanceByOne(Params.TabChars, Output);
	}

	void FieldAssignmentBeginning(StringView name)
	{
		INTRA_DEBUG_ASSERT(!name.Empty());
		CopyToAdvanceByOne(Lang.LeftFieldNameBeginQuote, Output);
		WriteTo(name, Output);
		CopyToAdvanceByOne(Lang.LeftFieldNameEndQuote, Output);

		if(Params.UseAssignmentSpaces)
			Output.Put(' ');

		CopyToAdvanceByOne(Lang.LeftAssignmentOperator, Output);

		if(Params.UseAssignmentSpaces && Lang.LeftAssignmentOperator!=null)
			Output.Put(' ');
	}

	void FieldAssignmentEnding(StringView name)
	{
		INTRA_PRECONDITION(name != null);

		if(Params.UseAssignmentSpaces)
			Output.Put(' ');

		CopyToAdvanceByOne(Lang.RightAssignmentOperator, Output);

		if(Params.UseAssignmentSpaces && Lang.RightAssignmentOperator!=null)
			Output.Put(' ');

		CopyToAdvanceByOne(Lang.RightFieldNameBeginQuote, Output);
		WriteTo(name, Output);
		CopyToAdvanceByOne(Lang.RightFieldNameEndQuote, Output);
	}

	void StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type=TextSerializerParams::TypeFlags_Struct)
	{
		auto openStr = (type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceOpen: Lang.TupleOpen;
		CopyToAdvanceByOne(openStr, Output);
		NestingLevel++;
		EndField(type);
	}

	void StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type=TextSerializerParams::TypeFlags_Struct)
	{
		NestingLevel--;
		EndField(type);
		auto closeStr = (type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceClose: Lang.TupleClose;
		CopyToAdvanceByOne(closeStr, Output);
	}

	//! Сериализовать кортеж
	template<typename Tuple> Requires<
		CHasForEachField<Tuple, GenericTextSerializerStructVisitor<O>> &&
		!CHasReflectionFieldNamesMethod<Tuple>
	> SerializeTuple(Tuple&& src, CSpan<StringView> fieldNames = null)
	{
		StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Tuple);
		if((Params.ValuePerLine & TextSerializerParams::TypeFlags_Tuple) == 0)
			fieldNames = null;
		GenericTextSerializerStructVisitor<O> visitor = {this,
			false, fieldNames, TextSerializerParams::TypeFlags_Tuple};
		ForEachField(src, visitor);
		StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Tuple);
	}


	//! Сериализатор как функтор
	template<typename T> INTRA_FORCEINLINE GenericTextSerializer& operator()(T&& v) {return *this << Forward<T>(v);}

	//! Вычислить размер объекта в сериализованном виде в байтах
	template<typename T> size_t SerializedSizeOf(T&& v) const
	{
		GenericTextSerializer<CountRange<char>> dummy(Lang, Params, CountRange<char>());
		dummy.NestingLevel = NestingLevel;
		dummy << Forward<T>(v);
		return dummy.Output.Counter;
	}

	void ResetOutput(const O& output) {Output=output; NestingLevel=0;}

	TextSerializerParams Params;
	LanguageParams Lang;
	int NestingLevel;
	O Output;
};

typedef GenericTextSerializer<SpanOutput<char>> TextSerializer;
typedef GenericTextSerializer<CountRange<char>> DummyTextSerializer;

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
		Me->FieldAssignmentBeginning(StringView(FieldNames.First()));

	*Me << Forward<T>(t);

	if(!FieldNames.Empty() && Me->Lang.AddFieldNameAfterAssignment)
		Me->FieldAssignmentEnding(StringView(FieldNames.First()));

	return *this;
}

//! Сериализовать структуру или класс со статической рефлексией
template<typename T, typename O> INTRA_FORCEINLINE Requires<
	CHasForEachField<T> &&
	CHasReflectionFieldNamesMethod<T>,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, T&& src)
{
	auto fieldNames = TRemoveConstRef<T>::ReflectionFieldNames();
	serializer.StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Struct);
	if(!serializer.Params.FieldAssignments && !serializer.Lang.RequireFieldAssignments)
		fieldNames = null;
	GenericTextSerializerStructVisitor<O> visitor = {&serializer,
		false, fieldNames, TextSerializerParams::TypeFlags_Struct};
	ForEachField(Forward<T>(src), visitor);
	serializer.StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Struct);
	return serializer;
}

//! Сериализовать кортеж
template<typename O, typename Tuple> Requires<
	CHasForEachField<Tuple, GenericTextSerializerStructVisitor<O>> &&
	!CHasReflectionFieldNamesMethod<Tuple>,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, Tuple&& src)
{
	serializer.SerializeTuple(Forward<Tuple>(src));
	return serializer;
}


//! Сериализация целых чисел
template<typename T, typename O> Requires<
	CIntegral<T>,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, T v)
{
	serializer.Output << v;
	return serializer;
}

//! Сериализация чисел с плавающей запятой
template<typename T, typename O> Requires<
	CFloatingPoint<T>,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, T v)
{
	ToString(serializer.Output, v, sizeof(T)<=4? 7: 15, serializer.Lang.DecimalSeparator);
	return serializer;
}

//! Сериализовать булевое значение
template<typename O> INTRA_FORCEINLINE GenericTextSerializer<O>& operator<<(
	GenericTextSerializer<O>& serializer, bool v)
{
	INTRA_DEBUG_ASSERT(int(v) <= 1 && "Invalid bool value!");
	CopyToAdvanceByOne(serializer.Lang.FalseTrueNames[v!=false], serializer.Output);
	return serializer;
}

//! Сериализовать диапазон
template<typename R, typename O> INTRA_FORCEINLINE Requires<
	CAsConsumableRange<R> &&
	!CAsCharRange<R>,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, R&& r)
{
	CopyToAdvanceByOne(serializer.Lang.ArrayOpen, serializer.Output);
	auto range = ForwardAsRange<R>(r);
	if(!range.Empty())
	{
		typedef TValueTypeOfAs<R> T;
		
		bool TypeIsSimpleArray = CArithmetic<T> ||
			CPointer<T> || CAsCharRange<T>;

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
	CopyToAdvanceByOne(serializer.Lang.ArrayClose, serializer.Output);
	return serializer;
}

template<typename O> GenericTextSerializer<O>& operator<<(GenericTextSerializer<O>& serializer, const StringView& v)
{
	CopyToAdvanceByOne(serializer.Lang.StringQuote, serializer.Output);
	MultiReplaceToAdvance(v, serializer.Output, Zip(
		CSpan<StringView>{"\n", "\r", "\t"},
		CSpan<StringView>{"\\n", "\\r", "\\t"}));
	CopyToAdvanceByOne(serializer.Lang.StringQuote, serializer.Output);
	return serializer;
}

//! Сериализовать массив фиксированной длины или строковой литерал
template<typename T, size_t N, typename O> INTRA_FORCEINLINE GenericTextSerializer<O>& operator<<(
	GenericTextSerializer<O>& serializer, const T(&src)[N])
{return serializer << SpanOf(src);}

INTRA_END
