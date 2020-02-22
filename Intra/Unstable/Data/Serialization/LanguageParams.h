#pragma once

#include "Core/Range/StringView.h"
#include "Core/Reflection.h"

INTRA_BEGIN
//! ���������, ������������ ����, � ������� � �� �������� ������������ ��������� ������������ � ��������������.
//! ��� ����������� ���������� �������������� ��������� ����������� ������� �� ������������ ������������ ������������� ����������� � ������������.
struct LanguageParams
{
	//! ������ ������������� ��������� � ���� ������������ � ������� ����� � ��������� ����� ��� ��������������.
	bool RequireFieldAssignments;

	//! ��������� �������� ���� ����� ��������.
	bool AddFieldNameAfterAssignment;
	
	//! ������ � �������� ������� ������������ ����.
	StringView LeftAssignmentOperator, RightAssignmentOperator;
	
	//! ����������� ����� ������.
	StringView TupleFieldSeparator, FieldSeparator;

	//! �� ��� ����������� ������ � ����� ����� ���� �� ������������.
	StringView LeftFieldNameBeginQuote, LeftFieldNameEndQuote;

	//! �� ��� ����������� ������ � ����� ����� ���� ����� ������������.
	StringView RightFieldNameBeginQuote, RightFieldNameEndQuote;
	
	//! ������� � �������� ������, ����� �������� ����� ����������� ���� �������.
	StringView TupleOpen, TupleClose;

	//! ������� � �������� ������, ����� �������� ����� ����������� ������������� ���� ���������.
	StringView StructInstanceOpen, StructInstanceClose;
	
	//! ������ ������ ������������� �����������.
	StringView OneLineCommentBegin;
	
	//! ������ � ����� �������������� �����������.
	StringView MultiLineCommentBegin, MultiLineCommentEnd;
	
	//! �������, � ������� ����������� ������.
	StringView StringQuote;

	//! �������, � ������� ����� ���� �������� ������.
	StringView CharQuotes;

	//! �������, ������������ ������ � ����� �������.
	StringView ArrayOpen, ArrayClose;

	//! ����������� ��������� �������.
	StringView ArrayElementSeparator;

	//! ������ � ����� ������������� ������.
	StringView MultiLineStringBegin, MultiLineStringEnd;

	//! ����������� ���������� �������� (�������� ["false", "true"]).
	StringView FalseTrueNames[2];

	//! ������ � �������� ����������� � ���������� �����.
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
