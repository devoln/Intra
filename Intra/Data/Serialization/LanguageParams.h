#pragma once

#include "Data/Reflection.h"
#include "Utils/StringView.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Структура, определяющая язык, в который и из которого производится текстовая сериализация и десериализация.
//! Для возможности корректной десериализации требуется внимательно следить за возможностью однозначного распознавания определений и разделителей.
struct LanguageParams
{
	//! Предопределённые описания языка сериализации:
	//! Инициализатор структур C99 с возможностью указания имён полей.
	static const LanguageParams CStructInitializer;

	//! Инициализатор структур языка D с возможностью указания имён полей.
	static const LanguageParams DStructInitializer;

	//! Кастомный язык для сериализации, похожий на JSON, но без кавычек и с оператором = вместо :.
	static const LanguageParams JsonLikeNoQuotes;

	//! Описание языка JSON.
	static const LanguageParams Json;

	//! Подмножество Xml, в котором используются вложенные узлы и не используются атрибуты.
	//! Каждая переменная описывается тегом с именем переменной.
	//! Структуры и кортежи обрамляются в теги <struct> и <tuple> соответственно.
	static const LanguageParams Xml;


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
