#pragma once

namespace Intra { namespace Data {

struct TextSerializerParams
{
	typedef flag8 TypeFlags;
	enum: TypeFlags {TypeFlags_None=0, TypeFlags_Number=1, TypeFlags_Char=2,
		TypeFlags_String=4, TypeFlags_Array=8, TypeFlags_Struct=16, TypeFlags_Tuple=32,
		TypeFlags_StructArray=64, TypeFlags_CustomType=128, TypeFlags_All=255};

	static const TextSerializerParams Verbose, VerboseNoSpaces, Compact, CompactSingleLine;

	//! Перед открытием определения экземпляра структуры записывать название типа
	bool UseStructInstanceTypeName;

	//! В определении экземпляра структуры указывать имена присваиваемых полей, а не просто перечисление значений.
	//! Если в DataLanguageParams выставлено RequireFieldAssignments, то это поле игнорируется и считается равным true
	bool FieldAssignments;

	//! Использовать пробелы вокруг знака присваивания
	bool UseAssignmentSpaces;

	//! Каждое значение на следующей строке. Иначе все значения в одну строку через пробел
	TypeFlags ValuePerLine;

	//! Для каких типов использовать отступы в строках для обозначения уровня вложенности
	TypeFlags UseTabs;

	//! Чем делать табуляцию. Например символом "\t", или пробелами "    ".
	StringView TabChars;

	//! Окончание строк. Обычно встречаются варианты: CR "\r", LF "\n" или CRLF "\r\n"
	StringView LineEnding;

	INTRA_ADD_REFLECTION(TextSerializerParams, UseStructInstanceTypeName, FieldAssignments,\
		UseAssignmentSpaces, ValuePerLine, UseTabs, TabChars, LineEnding);
};

struct DataLanguageParams
{
	static const DataLanguageParams CStructInitializer, DStructInitializer, JsonLikeNoQuotes, Json, Xml;

	//! Всегда сериализовать структуры в виде присваиваний с именами полей и требовать этого при десериализации
	bool RequireFieldAssignments;

	//! Приписать название поля после значения
	bool AddFieldNameAfterAssignment;
	
	//! Символ в качестве символа присваивания поля
	StringView LeftAssignmentOperator, RightAssignmentOperator;
	
	//! Разделитель между полями
	StringView TupleFieldSeparator, FieldSeparator;

	//! Во что обрамляются начало и конец имени поля до присваивания
	StringView LeftFieldNameBeginQuote, LeftFieldNameEndQuote;

	//! Во что обрамляются начало и конец имени поля после присваивания
	StringView RightFieldNameBeginQuote, RightFieldNameEndQuote;
	
	//! Символы в качестве скобок, между которыми будут перечислены поля кортежа
	StringView TupleOpen, TupleClose;

	//! Символы в качестве скобок, между которыми будут перечислены присваиваемые поля структуры
	StringView StructInstanceOpen, StructInstanceClose;
	
	//! Начало любого однострочного комментария
	StringView OneLineCommentBegin;
	
	//! Начало и конец многострочного комментария
	StringView MultiLineCommentBegin, MultiLineCommentEnd;
	
	//! Кавычки, в которые заключаются строки
	StringView StringQuote;

	//! Кавычки, в которые может быть заключён символ
	StringView CharQuotes;

	//! Символы, обозначающие начало и конец массива
	StringView ArrayOpen, ArrayClose;

	//! Разделитель элементов массива
	StringView ArrayElementSeparator;

	//! Начало и конец многострочной строки
	StringView MultiLineStringBegin, MultiLineStringEnd;

	//! Булевые значения
	StringView FalseTrueNames[2];

	//! Символ в качестве разделителя в десятичной дроби
	char DecimalSeparator;

	INTRA_ADD_REFLECTION(DataLanguageParams, RequireFieldAssignments, AddFieldNameAfterAssignment, \
		LeftAssignmentOperator, RightAssignmentOperator, TupleFieldSeparator, FieldSeparator, \
		LeftFieldNameBeginQuote, LeftFieldNameEndQuote, RightFieldNameBeginQuote, RightFieldNameEndQuote,\
		TupleOpen, TupleClose, StructInstanceOpen, StructInstanceClose, OneLineCommentBegin, \
		MultiLineCommentBegin, MultiLineCommentEnd, StringQuote, CharQuotes, \
		ArrayOpen, ArrayClose, ArrayElementSeparator, MultiLineStringBegin, MultiLineStringEnd, \
		FalseTrueNames, DecimalSeparator);
};


}}

