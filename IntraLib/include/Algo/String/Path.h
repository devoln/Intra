#pragma once

#include "Range/ForwardDecls.h"
#include "Containers/ForwardDeclarations.h"

namespace Intra { namespace Algo { namespace Path {

void NormalizeSlashesAndSpaces(ArrayRange<char>& path);
String AddTrailingSlash(StringView path);
StringView RemoveTrailingSlash(StringView path);

inline bool IsPathSeparator(char c) {return c=='/' || c=='\\';}
void SplitPath(StringView fullPath, StringView* oDirectoryPath,
	StringView* oNameOnly, StringView* oExtension, StringView* oName);
StringView ExtractDirectoryPath(StringView fullPath);
StringView ExtractNameWithoutExtension(StringView fullPath);
StringView ExtractName(StringView fullPath);
StringView ExtractExtension(StringView fullPath);

}}}
