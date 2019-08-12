#pragma once

#include "Core/Range/StringView.h"
#include "Container/ForwardDecls.h"


INTRA_BEGIN
namespace IO {

Array<byte> DownloadFile(StringView path);

}}

