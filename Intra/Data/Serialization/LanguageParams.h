#pragma once

#include "Data/Reflection.h"
#include "Utils/StringView.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! ���������, ������������ ����, � ������� � �� �������� ������������ ��������� ������������ � ��������������.
//! ��� ����������� ���������� �������������� ��������� ����������� ������� �� ������������ ������������ ������������� ����������� � ������������.
struct LanguageParams
{
	//! ��������������� �������� ����� ������������:
	//! ������������� �������� C99 � ������������ �������� ��� �����.
	static const LanguageParams CStructInitializer;

	//! ������������� �������� ����� D � ������������ �������� ��� �����.
	static const LanguageParams DStructInitializer;

	//! ��������� ���� ��� ������������, ������� �� JSON, �� ��� ������� � � ���������� = ������ :.
	static const LanguageParams JsonLikeNoQuotes;

	//! �������� ����� JSON.
	static const LanguageParams Json;

	//! ������������ Xml, � ������� ������������ ��������� ���� � �� ������������ ��������.
	//! ������ ���������� ����������� ����� � ������ ����������.
	//! ��������� � ������� ����������� � ���� <struct> � <tuple> ��������������.
	static const LanguageParams Xml;


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

	INTRA_ADD_REFLECTION(LanguageParams, RequireFieldAssignments, AddFieldNameAfterAssignment, \
		LeftAssignmentOperator, RightAssignmentOperator, TupleFieldSeparator, FieldSeparator, \
		LeftFieldNameBeginQuote, LeftFieldNameEndQuote, RightFieldNameBeginQuote, RightFieldNameEndQuote,\
		TupleOpen, TupleClose, StructInstanceOpen, StructInstanceClose, OneLineCommentBegin, \
		MultiLineCommentBegin, MultiLineCommentEnd, StringQuote, CharQuotes, \
		ArrayOpen, ArrayClose, ArrayElementSeparator, MultiLineStringBegin, MultiLineStringEnd, \
		FalseTrueNames, DecimalSeparator)
};

INTRA_WARNING_POP

}}
