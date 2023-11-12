#pragma once

#include "Intra/Core.h"
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

namespace Intra { INTRA_BEGIN

template<typename O> class GenericTextSerializer;

template<typename O> struct GenericTextSerializerStructVisitor
{
	GenericTextSerializer<O>* Me;
	bool Began;
	Span<const const char*> FieldNames;
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
		if(typeFlag & TextSerializerParams::TypeFlags::Struct)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags::Struct);
			CopyToAdvanceByOne(Lang.FieldSeparator, Output);
		}
		if(typeFlag & TextSerializerParams::TypeFlags::Tuple)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags::Tuple);
			CopyToAdvanceByOne(Lang.TupleFieldSeparator, Output);
		}
		if(typeFlag & TextSerializerParams::TypeFlags::Array)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags::Array);
			CopyToAdvanceByOne(Lang.ArrayElementSeparator, Output);
		}
		if(typeFlag & TextSerializerParams::TypeFlags::StructArray)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags::StructArray);
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
		if((Params.Indent & typeFlag) == TextSerializerParams::TypeFlags::None) return;
		for(int i = 0; i < NestingLevel; i++)
			CopyToAdvanceByOne(Params.IndentationLevel, Output);
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

		if(Params.UseAssignmentSpaces && !Lang.LeftAssignmentOperator.Empty())
			Output.Put(' ');
	}

	void FieldAssignmentEnding(StringView name)
	{
		INTRA_PRECONDITION(!name.Empty());

		if(Params.UseAssignmentSpaces)
			Output.Put(' ');

		CopyToAdvanceByOne(Lang.RightAssignmentOperator, Output);

		if(Params.UseAssignmentSpaces && Lang.RightAssignmentOperator!=nullptr)
			Output.Put(' ');

		CopyToAdvanceByOne(Lang.RightFieldNameBeginQuote, Output);
		WriteTo(name, Output);
		CopyToAdvanceByOne(Lang.RightFieldNameEndQuote, Output);
	}

	void StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type=TextSerializerParams::TypeFlags::Struct)
	{
		auto openStr = (type & TextSerializerParams::TypeFlags::Struct)? Lang.StructInstanceOpen: Lang.TupleOpen;
		CopyToAdvanceByOne(openStr, Output);
		NestingLevel++;
		EndField(type);
	}

	void StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type=TextSerializerParams::TypeFlags::Struct)
	{
		NestingLevel--;
		EndField(type);
		auto closeStr = (type & TextSerializerParams::TypeFlags::Struct)? Lang.StructInstanceClose: Lang.TupleClose;
		CopyToAdvanceByOne(closeStr, Output);
	}

	/// Сериализовать кортеж
	template<CHasForEachField<GenericTextSerializerStructVisitor<O>> Tuple> requires (!CHasReflectionFieldNamesMethod<Tuple>)
	void SerializeTuple(Tuple&& src, Span<const StringView> fieldNames = {})
	{
		StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags::Tuple);
		if((Params.ValuePerLine & TextSerializerParams::TypeFlags::Tuple) == 0)
			fieldNames = nullptr;
		GenericTextSerializerStructVisitor<O> visitor = {this,
			false, fieldNames, TextSerializerParams::TypeFlags::Tuple};
		ForEachField(src, visitor);
		StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags::Tuple);
	}


	/// Сериализатор как функтор
	template<typename T> INTRA_FORCEINLINE GenericTextSerializer& operator()(T&& v) {return *this << INTRA_FWD(v);}

	/// Вычислить размер объекта в сериализованном виде в байтах
	template<typename T> size_t SerializedSizeOf(T&& v) const
	{
		GenericTextSerializer<RCounter<char>> dummy(Lang, Params, RCounter<char>());
		dummy.NestingLevel = NestingLevel;
		dummy << INTRA_FWD(v);
		return dummy.Output.Counter;
	}

	void ResetOutput(const O& output) {Output = output; NestingLevel = 0;}

	TextSerializerParams Params;
	LanguageParams Lang;
	int NestingLevel;
	O Output;
};

using TextSerializer = GenericTextSerializer<SpanOutput<char>>;
using DummyTextSerializer = GenericTextSerializer<RCounter<char>>;

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

	*Me << INTRA_FWD(t);

	if(!FieldNames.Empty() && Me->Lang.AddFieldNameAfterAssignment)
		Me->FieldAssignmentEnding(StringView(FieldNames.First()));

	return *this;
}

/// Сериализовать структуру или класс со статической рефлексией
template<typename T, typename O> INTRA_FORCEINLINE Requires<
	CHasForEachField<T> &&
	CHasReflectionFieldNamesMethod<T>,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, T&& src)
{
	auto fieldNames = TRemoveConstRef<T>::ReflectionFieldNames();
	serializer.StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags::Struct);
	if(!serializer.Params.FieldAssignments && !serializer.Lang.RequireFieldAssignments)
		fieldNames = nullptr;
	GenericTextSerializerStructVisitor<O> visitor = {&serializer,
		false, fieldNames, TextSerializerParams::TypeFlags::Struct};
	INTRA_FWD(src)|ForEachField(visitor);
	serializer.StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags::Struct);
	return serializer;
}

/// Сериализовать кортеж
template<typename O, typename Tuple> Requires<
	CHasForEachField<Tuple, GenericTextSerializerStructVisitor<O>> &&
	!CHasReflectionFieldNamesMethod<Tuple>,
GenericTextSerializer<O>&> operator<<(GenericTextSerializer<O>& serializer, Tuple&& src)
{
	serializer.SerializeTuple(Forward<Tuple>(src));
	return serializer;
}


/// Сериализация целых чисел
template<CBasicIntegral T, typename O> GenericTextSerializer<O>& operator<<(GenericTextSerializer<O>& serializer, T v)
{
	serializer.Output << v;
	return serializer;
}

/// Сериализация чисел с плавающей запятой
template<CBasicFloatingPoint T, typename O> GenericTextSerializer<O>& operator<<(GenericTextSerializer<O>& serializer, T v)
{
	ToString(serializer.Output, v, sizeof(T) <= 4? 7: 15, serializer.Lang.DecimalSeparator);
	return serializer;
}

/// Сериализовать булевое значение
template<typename O> INTRA_FORCEINLINE GenericTextSerializer<O>& operator<<(GenericTextSerializer<O>& serializer, bool v)
{
	INTRA_DEBUG_ASSERT(int(v) <= 1 && "invalid bool value!");
	CopyToAdvanceByOne(serializer.Lang.FalseTrueNames[v], serializer.Output);
	return serializer;
}

/// Сериализовать диапазон
template<CConsumableList L, typename O> requires (!CCharList<R>)
INTRA_FORCEINLINE GenericTextSerializer<O>& operator<<(GenericTextSerializer<O>& serializer, L&& list)
{
	CopyToAdvanceByOne(serializer.Lang.ArrayOpen, serializer.Output);
	auto range = RangeOf(INTRA_FWD(list));
	if(!range.Empty())
	{
		using T = TListValue<L>;
		
		constexpr bool TypeIsSimpleArray = CBasicArithmetic<T> || CBasicPointer<T> || CCharList<T>;

		const int nestLevelUp = int(!TypeIsSimpleArray && !serializer.Lang.ArrayOpen.Empty() && !serializer.Lang.ArrayClose.Empty());
		serializer.NestingLevel += nestLevelUp;
		if(!TypeIsSimpleArray && !serializer.Lang.StructInstanceOpen.Empty())
			serializer.EndField(TextSerializerParams::TypeFlags::StructArray);

		serializer << range.First();
		range.PopFirst();
		while(!range.Empty())
		{
			serializer.NextField(TypeIsSimpleArray?
				TextSerializerParams::TypeFlags::Array:
				TextSerializerParams::TypeFlags::StructArray);
			serializer << range.First();
			range.PopFirst();
		}

		serializer.NestingLevel -= nestLevelUp;
		if(!TypeIsSimpleArray && !serializer.Lang.StructInstanceClose.Empty())
			serializer.EndField(TextSerializerParams::TypeFlags::StructArray);
	}
	CopyToAdvanceByOne(serializer.Lang.ArrayClose, serializer.Output);
	return serializer;
}

template<typename O> GenericTextSerializer<O>& operator<<(GenericTextSerializer<O>& serializer, const StringView& v)
{
	CopyToAdvanceByOne(serializer.Lang.StringQuote, serializer.Output);
	MultiReplaceToAdvance(v, serializer.Output, Zip(
		Span<const StringView>{"\n", "\r", "\t"},
		Span<const StringView>{"\\n", "\\r", "\\t"}));
	CopyToAdvanceByOne(serializer.Lang.StringQuote, serializer.Output);
	return serializer;
}

/// Сериализовать массив фиксированной длины или строковой литерал
template<typename T, size_t N, typename O> INTRA_FORCEINLINE GenericTextSerializer<O>& operator<<(
	GenericTextSerializer<O>& serializer, const T(&src)[N])
{return serializer << Span(src);}

} INTRA_END
