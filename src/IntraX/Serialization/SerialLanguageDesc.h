#pragma once

#include <Intra/Concepts.h>
#include <Intra/Container/String.h>

namespace Intra { INTRA_BEGIN
/// Text serialization language definition.
/// In order to allow deserialization, this definition must not be ambiguous.
struct SerialLanguageDesc
{
	bool RequireFieldAssignments: 1; // serialize structures with field names and require them on deserialization
	bool AddFieldNameAfterAssignment: 1;
	bool AllowExponentialNumberNotation: 1;

	char DecimalSeparator;
	char ThousandSeparator; // or '\0' if none
	
	String LeftAssignmentOperator, RightAssignmentOperator;
	String TupleFieldSeparator, FieldSeparator;
	String LeftFieldNameBeginQuote, LeftFieldNameEndQuote;
	String RightFieldNameBeginQuote, RightFieldNameEndQuote;
	String TupleOpen, TupleClose;
	String StructInstanceOpen, StructInstanceClose;
	String OneLineCommentBegin;
	String MultiLineCommentBegin, MultiLineCommentEnd;
	String StringQuote;
	String CharQuotes;
	String ArrayOpen, ArrayClose;
	String ArrayElementSeparator;
	String MultiLineStringBegin, MultiLineStringEnd;
	Array<String, 2> FalseTrueNames;
};
INTRA_ADD_FIELD_REFLECTION(SerialLanguageDesc,
	RequireFieldAssignments, AddFieldNameAfterAssignment, AllowExponentialNumberNotation, DecimalSeparator, ThousandSeparator,
	LeftAssignmentOperator, RightAssignmentOperator, TupleFieldSeparator, FieldSeparator,
	LeftFieldNameBeginQuote, LeftFieldNameEndQuote, RightFieldNameBeginQuote, RightFieldNameEndQuote,
	TupleOpen, TupleClose, StructInstanceOpen, StructInstanceClose, OneLineCommentBegin,
	MultiLineCommentBegin, MultiLineCommentEnd, StringQuote, CharQuotes,
	ArrayOpen, ArrayClose, ArrayElementSeparator, MultiLineStringBegin, MultiLineStringEnd,
	FalseTrueNames);

/// Predefined serialization language descriptions:
struct SerialLanguageDescs
{
	/// C99 struct initializer with optional field names (designated initializers).
	static constexpr SerialLanguageDesc CStructInitializer =
	{
		.RequireFieldAssignments = false,
		.AddFieldNameAfterAssignment = false,
		.AllowExponentialNumberNotation = true,
		.DecimalSeparator = '.',
		.ThousandSeparator = 0,

		.LeftAssignmentOperator = "=",
		.RightAssignmentOperator = "",
		.TupleFieldSeparator = ",",
		.FieldSeparator = ",",
		.LeftFieldNameBeginQuote = ".",
		.LeftFieldNameEndQuote = "",
		.RightFieldNameBeginQuote = "",
		.RightFieldNameEndQuote = "",
		.TupleOpen = "{",
		.TupleClose = "}",
		.StructInstanceOpen = "{",
		.StructInstanceClose = "}",
		.OneLineCommentBegin = "//",
		.MultiLineCommentBegin = "/*",
		.MultiLineCommentEnd = "*/",
		.StringQuote = "\"",
		.CharQuotes = "'",
		.ArrayOpen = "{",
		.ArrayClose = "}",
		.ArrayElementSeparator = ",",
		.MultiLineStringBegin = "R\"(",
		.MultiLineStringEnd = ")\"",
		.FalseTrueNames = {"false", "true"}
	};

	/// D language struct intiailizer with optional field names.
	static constexpr SerialLanguageDesc DStructInitializer =
	{
		.RequireFieldAssignments = false,
		.AddFieldNameAfterAssignment = false,
		.AllowExponentialNumberNotation = true,
		.DecimalSeparator = '.',
		.ThousandSeparator = 0,

		.LeftAssignmentOperator = ":",
		.RightAssignmentOperator = "",
		.TupleFieldSeparator = ",",
		.FieldSeparator = ",",
		.LeftFieldNameBeginQuote = "",
		.LeftFieldNameEndQuote = "",
		.RightFieldNameBeginQuote = "",
		.RightFieldNameEndQuote = "",
		.TupleOpen = "[",
		.TupleClose = "]",
		.StructInstanceOpen = "{",
		.StructInstanceClose = "}",
		.OneLineCommentBegin = "//",
		.MultiLineCommentBegin = "/*",
		.MultiLineCommentEnd = "*/",
		.StringQuote = "\"",
		.CharQuotes = "'",
		.ArrayOpen = "[",
		.ArrayClose = "]",
		.ArrayElementSeparator = ",",
		.MultiLineStringBegin = "r\"",
		.MultiLineStringEnd = "\"",
		.FalseTrueNames = {"false", "true"}
	};

	/// JSON-like custom language without quotes and with '=' instead of ':'.
	static constexpr SerialLanguageDesc JsonLikeNoQuotes =
	{
		.RequireFieldAssignments = false,
		.AddFieldNameAfterAssignment = false,
		.AllowExponentialNumberNotation = true,
		.DecimalSeparator = '.',
		.ThousandSeparator = 0,

		.LeftAssignmentOperator = "=",
		.RightAssignmentOperator = "",
		.TupleFieldSeparator = ",",
		.FieldSeparator = ",",
		.LeftFieldNameBeginQuote = "",
		.LeftFieldNameEndQuote = "",
		.RightFieldNameBeginQuote = "",
		.RightFieldNameEndQuote = "",
		.TupleOpen = "[",
		.TupleClose = "]",
		.StructInstanceOpen = "{",
		.StructInstanceClose = "}",
		.OneLineCommentBegin = "//",
		.MultiLineCommentBegin = "/*",
		.MultiLineCommentEnd = "*/",
		.StringQuote = "\"",
		.CharQuotes = "'",
		.ArrayOpen = "[",
		.ArrayClose = "]",
		.ArrayElementSeparator = ",",
		.MultiLineStringBegin = "{[",
		.MultiLineStringEnd = "}]",
		.FalseTrueNames = {"false", "true"}
	};

	/// JSON language description.
	static constexpr SerialLanguageDesc Json =
	{
		.RequireFieldAssignments = false,
		.AddFieldNameAfterAssignment = false,
		.AllowExponentialNumberNotation = true,
		.DecimalSeparator = '.',
		.ThousandSeparator = 0,

		.LeftAssignmentOperator = ":",
		.RightAssignmentOperator = "",
		.TupleFieldSeparator = ",",
		.FieldSeparator = ",",
		.LeftFieldNameBeginQuote = "\"",
		.LeftFieldNameEndQuote = "\"",
		.RightFieldNameBeginQuote = "\"",
		.RightFieldNameEndQuote = "\"",
		.TupleOpen = "[",
		.TupleClose = "]",
		.StructInstanceOpen = "{",
		.StructInstanceClose = "}",

		// NOTE: comments are not a part of JSON spec, supported as an extension
		.OneLineCommentBegin = "//",
		.MultiLineCommentBegin = "/*",
		.MultiLineCommentEnd = "*/",

		.StringQuote = "\"",
		.CharQuotes = "'",
		.ArrayOpen = "[",
		.ArrayClose = "]",
		.ArrayElementSeparator = ",",
		.FalseTrueNames = {"false", "true"}
	};

	/// Very limited XML subset-like language, using inner nodes without attributes.
	/// Each variable is described by a tag with its name.
	/// Structures and tuples are surrounded by <struct> and <tuple> respectively.
	static constexpr SerialLanguageDesc Xml =
	{
		.RequireFieldAssignments = true,
		.AddFieldNameAfterAssignment = true,
		.AllowExponentialNumberNotation = true,
		.DecimalSeparator = '.',
		.ThousandSeparator = 0,

		.LeftAssignmentOperator = "",
		.RightAssignmentOperator = "",
		.TupleFieldSeparator = "",
		.FieldSeparator = "",
		.LeftFieldNameBeginQuote = "<",
		.LeftFieldNameEndQuote = ">",
		.RightFieldNameBeginQuote = "</",
		.RightFieldNameEndQuote = ">",
		.TupleOpen = "<tuple>",
		.TupleClose = "</tuple>",
		.StructInstanceOpen = "<struct>",
		.StructInstanceClose = "</struct>",

		.MultiLineCommentBegin = "<!--",
		.MultiLineCommentEnd = "-->",

		.StringQuote = "\"",
		.CharQuotes = "'",
		.MultiLineStringBegin = "<text>",
		.MultiLineStringEnd = "</text>",
		.FalseTrueNames = {"false", "true"}
	};
};

} INTRA_END
