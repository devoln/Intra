#pragma once

#include "Utils/StringView.h"
#include "Container/ForwardDecls.h"


namespace Intra { namespace IO {

Array<byte> DownloadFile(StringView path);

}}

