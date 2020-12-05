#pragma once

#include "Intra/Range/Span.h"
#include "Intra/Range/StringView.h"
#include "IntraX/Container/ForwardDecls.h"

INTRA_BEGIN
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
StringView GetParentPath(StringView path);
bool IsAbsolutePath(StringView path);
INTRA_END
