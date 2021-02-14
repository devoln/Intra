#pragma once

#include "Intra/Core.h"
#include "Intra/Range/StringView.h"
#include "IntraX/Container/ForwardDecls.h"

namespace Intra { INTRA_BEGIN
Array<byte> DownloadFile(StringView path);
} INTRA_END
