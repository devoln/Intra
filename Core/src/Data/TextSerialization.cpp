#include "Data/Serialization/TextSerializerParams.h"

namespace Intra { namespace Data {

const TextSerializerParams TextSerializerParams::Verbose =
{
	false,
	true,
	true,
	TypeFlags_Struct|TypeFlags_StructArray,
	TypeFlags_Struct|TypeFlags_StructArray,
	"\t",
	"\r\n"
};

const TextSerializerParams TextSerializerParams::VerboseNoSpaces =
{
	false,
	true,
	false,
	TypeFlags_Struct|TypeFlags_StructArray,
	TypeFlags_Struct|TypeFlags_StructArray,
	"\t",
	"\r\n"
};

const TextSerializerParams TextSerializerParams::Compact =
{
	false,
	false,
	true,
	TypeFlags_StructArray,
	TypeFlags_StructArray,
	"\t",
	"\r\n"
};

const TextSerializerParams TextSerializerParams::CompactSingleLine =
{
	false,
	false,
	false,
	TypeFlags_None,
	TypeFlags_None,
	"\t",
	"\r\n"
};

const DataLanguageParams DataLanguageParams::CStructInitializer =
{
	false, false,
	"=", "",
	",", ",",
	".", "",
	"", "",
	"{", "}",
	"{", "}",
	"//",
	"/*", "*/",
	"\"",
	"'",
	"{", "}",
	",",
	"R\"(", ")\"",
	{"false", "true"},
	'.'
};

const DataLanguageParams DataLanguageParams::DStructInitializer =
{
	false, false,
	":", "",
	",", ",",
	"", "",
	"", "",
	"[", "]",
	"{", "}",
	"//",
	"/*", "*/",
	"\"",
	"'",
	"[", "]",
	",",
	"r\"", "\"",
	{"false", "true"},
	'.'
};

const DataLanguageParams DataLanguageParams::JsonLikeNoQuotes =
{
	false, false,
	"=", "",
	",", ",",
	"", "",
	"", "",
	"[", "]",
	"{", "}",
	"//",
	"/*", "*/",
	"\"",
	"'",
	"[", "]",
	",",
	"{[", "]}",
	{"false", "true"},
	'.'
};

const DataLanguageParams DataLanguageParams::Json =
{
	false, false,
	":", "",
	",", ",",
	"\"", "\"",
	"\"", "\"",
	"[", "]",
	"{", "}",
	"//",
	"/*", "*/",
	"\"",
	"'",
	"[", "]",
	",",
	"{[", "]}",
	{"false", "true"},
	'.'
};

const DataLanguageParams DataLanguageParams::Xml =
{
	true, true,
	"", "",
	"", "",
	"<", ">",
	"</", ">",
	"<tuple>", "</tuple>",
	"<struct>", "</struct>",
	"",
	"<!--", "-->",
	"\"",
	"'",
	"", "",
	"",
	"<text>", "</text>",
	{"false", "true"},
	'.'
};


	


}}
