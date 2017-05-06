#pragma once

#include "Utils/Span.h"
#include "Utils/StringView.h"
#include "Container/ForwardDecls.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace IO { namespace Path {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void NormalizeSlashesAndSpaces(Span<char>& path);
String AddTrailingSlash(StringView path);
StringView RemoveTrailingSlash(StringView path);

inline bool IsPathSeparator(char c) {return c=='/' || c=='\\';}
void SplitPath(StringView fullPath, StringView* oDirectoryPath,
	StringView* oNameOnly, StringView* oExtension, StringView* oName);
StringView ExtractDirectoryPath(StringView fullPath);
StringView ExtractNameWithoutExtension(StringView fullPath);
StringView ExtractName(StringView fullPath);
StringView ExtractExtension(StringView fullPath);

INTRA_WARNING_POP

}}}
