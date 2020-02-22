#pragma once

#include "Core/Reflection.h"
#include "Core/Range/StringView.h"

INTRA_BEGIN
//! Parameters for text serialization formatting.
//! This parameters doesn't affect deserialization.
struct TextSerializerParams
{
	typedef byte TypeFlags;
	enum: TypeFlags {
		TypeFlags_None = 0,
		TypeFlags_Number = 1,
		TypeFlags_Char = 2,
		TypeFlags_String = 4,
		TypeFlags_Array = 8,
		TypeFlags_Struct = 16,
		TypeFlags_Tuple = 32,
		TypeFlags_StructArray = 64,
		TypeFlags_CustomType = 128,
		
		TypeFlags_All = 255
	};

	//! В определении экземпляра структуры указывать имена присваиваемых полей, а не просто перечисление значений.
	//! Если в LanguageParams выставлено RequireFieldAssignments, то это поле игнорируется и считается равным true
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

	//INTRA_ADD_FIELD_REFLECTION(TextSerializerParams, FieldAssignments, UseAssignmentSpaces, ValuePerLine, UseTabs, TabChars, LineEnding);
};

struct TextSerializerParamsBuiltin
{
	static constexpr TextSerializerParams Verbose =
	{
		true,
		true,
		TextSerializerParams::TypeFlags_Struct|TextSerializerParams::TypeFlags_StructArray,
		TextSerializerParams::TypeFlags_Struct|TextSerializerParams::TypeFlags_StructArray,
		"\t"_v,
		"\r\n"_v
	};

	static constexpr TextSerializerParams VerboseNoSpaces =
	{
		true,
		false,
		TextSerializerParams::TypeFlags_Struct|TextSerializerParams::TypeFlags_StructArray,
		TextSerializerParams::TypeFlags_Struct|TextSerializerParams::TypeFlags_StructArray,
		"\t"_v,
		"\r\n"_v
	};

	static constexpr TextSerializerParams Compact =
	{
		false,
		true,
		TextSerializerParams::TypeFlags_StructArray,
		TextSerializerParams::TypeFlags_StructArray,
		"\t"_v,
		"\r\n"_v
	};

	static constexpr TextSerializerParams CompactSingleLine =
	{
		false,
		false,
		TextSerializerParams::TypeFlags_None,
		TextSerializerParams::TypeFlags_None,
		"\t"_v,
		"\r\n"_v
	};
};
INTRA_END
