#pragma once

#include "Intra/Reflection.h"
#include "Intra/Range/StringView.h"

namespace Intra { INTRA_BEGIN
/// Parameters for text serialization formatting.
/// This parameters doesn't affect deserialization.
struct TextSerializerParams
{
	enum TypeFlags {
		None = 0,
		Number = 1,
		Char = 2,
		String = 4,
		Array = 8,
		Struct = 16,
		Tuple = 32,
		StructArray = 64,
		CustomType = 128,
		
		All = 255,

		TagGenSameTypeBitOps
	};

	/// В определении экземпляра структуры указывать имена присваиваемых полей, а не просто перечисление значений.
	/// Если в LanguageParams выставлено RequireFieldAssignments, то это поле игнорируется и считается равным true
	bool FieldAssignments: 1;

	/// Spaces around assignment
	bool UseAssignmentSpaces: 1;

	/// Types that require one value per line
	TypeFlags ValuePerLine;

	/// Which types need indentation to show nesting levels
	TypeFlags Indent;

	/// e.g. "\t" or "    ".
	StringView IndentationLevel;

	/// e.g. "\r", "\n" or "\r\n"
	StringView LineEnding;
};
//INTRA_ADD_FIELD_REFLECTION(TextSerializerParams, FieldAssignments, UseAssignmentSpaces, ValuePerLine, UseTabs, TabChars, LineEnding);

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
} INTRA_END
