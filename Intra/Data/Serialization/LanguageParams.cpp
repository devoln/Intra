#include "Data/Serialization/LanguageParams.h"

INTRA_BEGIN
namespace Data {

const LanguageParams LanguageParams::CStructInitializer =
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

const LanguageParams LanguageParams::DStructInitializer =
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

const LanguageParams LanguageParams::JsonLikeNoQuotes =
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

const LanguageParams LanguageParams::Json =
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

const LanguageParams LanguageParams::Xml =
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
