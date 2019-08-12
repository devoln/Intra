#include "Data/Serialization/TextSerializerParams.h"

INTRA_BEGIN
namespace Data {

const TextSerializerParams TextSerializerParams::Verbose =
{
	true,
	true,
	TypeFlags_Struct|TypeFlags_StructArray,
	TypeFlags_Struct|TypeFlags_StructArray,
	"\t",
	"\r\n"
};

const TextSerializerParams TextSerializerParams::VerboseNoSpaces =
{
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
	TypeFlags_None,
	TypeFlags_None,
	"\t",
	"\r\n"
};

}}
