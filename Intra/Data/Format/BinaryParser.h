#pragma once

#include "Data/Serialization/BinaryDeserializer.h"
#include "BinaryRaw.h"

namespace Intra { namespace Data { namespace Format {

class BinaryParser
{
public:
	forceinline BinaryParser(CSpan<byte> data): mStream(data) {}
	
private:
	CSpan<byte> mStream;
};

}}}
