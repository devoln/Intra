#pragma once

#include "Intra/Core.h"
#include "Intra/Range/StringView.h"
#include "IntraX/Container/ForwardDecls.h"

INTRA_BEGIN
Array<byte> DownloadFile(StringView path);
INTRA_END
