#pragma once

#include "Core/Core.h"
#include "Core/Range/StringView.h"
#include "Container/ForwardDecls.h"

INTRA_BEGIN
Array<byte> DownloadFile(StringView path);
INTRA_END
