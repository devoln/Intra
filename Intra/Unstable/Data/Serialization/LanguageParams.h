#pragma once

#include "Core/Range/StringView.h"
#include "Core/Reflection.h"

INTRA_BEGIN
//! Структура, определяющая язык, в который и из которого производится текстовая сериализация и десериализация.
//! Для возможности корректной десериализации требуется внимательно следить за возможностью однозначного распознавания определений и разделителей.
struct LanguageParams
{
	//! Всегда сериализовать структуры в виде присваиваний с именами полей и требовать этого при десериализации.
	bool RequireFieldAssignments;

	//! Приписать название поля после значения.
	bool AddFieldNameAfterAssignment;
	
	//! Символ в качестве символа присваивания поля.
	StringView LeftAssignmentOperator, RightAssignmentOperator;
	
	//! Разделитель между полями.
	StringView TupleFieldSeparator, FieldSeparator;

	//! Во что обрамляются начало и конец имени поля до присваивания.
	StringView LeftFieldNameBeginQuote, LeftFieldNameEndQuote;

	//! Во что обрамляются начало и конец имени поля после присваивания.
	StringView RightFieldNameBeginQuote, RightFieldNameEndQuote;
	
	//! Символы в качестве скобок, между которыми будут перечислены поля кортежа.
	StringView TupleOpen, TupleClose;

	//! Символы в качестве скобок, между которыми будут перечислены присваиваемые поля структуры.
	StringView StructInstanceOpen, StructInstanceClose;
	
	//! Начало любого однострочного комментария.
	StringView OneLineCommentBegin;
	
	//! Начало и конец многострочного комментария.
	StringView MultiLineCommentBegin, MultiLineCommentEnd;
	
	//! Кавычки, в которые заключаются строки.
	StringView StringQuote;

	//! Кавычки, в которые может быть заключён символ.
	StringView CharQuotes;

	//! Символы, обозначающие начало и конец массива.
	StringView ArrayOpen, ArrayClose;

	//! Разделитель элементов массива.
	StringView ArrayElementSeparator;

	//! Начало и конец многострочной строки.
	StringView MultiLineStringBegin, MultiLineStringEnd;

	//! Обозначение логических значений (например ["false", "true"]).
	StringView FalseTrueNames[2];

	//! Символ в качестве разделителя в десятичной дроби.
	char DecimalSeparator;

	INTRA_ADD_FIELD_REFLECTION(LanguageParams, RequireFieldAssignments, AddFieldNameAfterAssignment,
		LeftAssignmentOperator, RightAssignmentOperator, TupleFieldSeparator, FieldSeparator,
		LeftFieldNameBeginQuote, LeftFieldNameEndQuote, RightFieldNameBeginQuote, RightFieldNameEndQuote,
		TupleOpen, TupleClose, StructInstanceOpen, StructInstanceClose, OneLineCommentBegin,
		MultiLineCommentBegin, MultiLineCommentEnd, StringQuote, CharQuotes,
		ArrayOpen, ArrayClose, ArrayElementSeparator, MultiLineStringBegin, MultiLineStringEnd,
		FalseTrueNames, DecimalSeparator)
};

//! Predefined serialization language descriptions:
struct LanguageParamsBuiltin
{
	//! C99 struct initializer with optional field names (designated initializers).
	static constexpr LanguageParams CStructInitializer =
	{
		false, false,
		"="_v, ""_v,
		","_v, ","_v,
		"."_v, ""_v,
		""_v, ""_v,
		"{"_v, "}"_v,
		"{"_v, "}"_v,
		"//"_v,
		"/*"_v, "*/"_v,
		"\""_v,
		"'"_v,
		"{"_v, "}"_v,
		","_v,
		"R\"("_v, ")\""_v,
		{"false"_v, "true"_v},
		'.'
	};

	//! D language struct intiailizer with optional field names.
	static constexpr LanguageParams DStructInitializer =
	{
		false, false,
		":"_v, ""_v,
		","_v, ","_v,
		""_v, ""_v,
		""_v, ""_v,
		"["_v, "]"_v,
		"{"_v, "}"_v,
		"//"_v,
		"/*"_v, "*/"_v,
		"\""_v,
		"'"_v,
		"["_v, "]"_v,
		","_v,
		"r\""_v, "\""_v,
		{"false"_v, "true"_v},
		'.'
	};

	//! JSON-like custom language without quotes and with '=' instead of ':'.
	static constexpr LanguageParams JsonLikeNoQuotes =
	{
		false, false,
		"="_v, ""_v,
		","_v, ","_v,
		""_v, ""_v,
		""_v, ""_v,
		"["_v, "]"_v,
		"{"_v, "}"_v,
		"//"_v,
		"/*"_v, "*/"_v,
		"\""_v,
		"'"_v,
		"["_v, "]"_v,
		","_v,
		"{["_v, "]}"_v,
		{"false"_v, "true"_v},
		'.'
	};

	//! JSON language description.
	static constexpr LanguageParams Json =
	{
		false, false,
		":"_v, ""_v,
		","_v, ","_v,
		"\""_v, "\""_v,
		"\""_v, "\""_v,
		"["_v, "]"_v,
		"{"_v, "}"_v,
		"//"_v,
		"/*"_v, "*/"_v,
		"\""_v,
		"'"_v,
		"["_v, "]"_v,
		","_v,
		"{["_v, "]}"_v,
		{"false"_v, "true"_v},
		'.'
	};

	//! Xml subset, using inner nodes without attributes.
	//! Each variable is described by a tag with its name.
	//! Structures and tuples are surrounded by <struct> and <tuple> respectively.
	static constexpr LanguageParams Xml =
	{
		true, true,
		""_v, ""_v,
		""_v, ""_v,
		"<"_v, ">"_v,
		"</"_v, ">"_v,
		"<tuple>"_v, "</tuple>"_v,
		"<struct>"_v, "</struct>"_v,
		""_v,
		"<!--"_v, "-->"_v,
		"\""_v,
		"'"_v,
		""_v, ""_v,
		""_v,
		"<text>"_v, "</text>"_v,
		{"false"_v, "true"_v},
		'.'
	};
};

INTRA_END
