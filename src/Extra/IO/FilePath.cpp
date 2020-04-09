#include "FilePath.h"
#include "Intra/Functional.h"
#include "Intra/Range/Mutation/Replace.h"
#include "Intra/Range/Retro.h"
#include "Intra/Range/Search/Trim.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/Span.h"
#include "Extra/Container/Sequential/String.h"

INTRA_BEGIN
void NormalizeSlashesAndSpaces(Span<char>& path)
{
	TrimAdvance(path, IsSpace);
	Replace(path, '\\', '/');
}

String AddTrailingSlash(StringView path)
{return IsAnySlash(path.Last())? String(path): path+'/';}

StringView RemoveTrailingSlash(StringView path)
{return TrimRight(path, IsPathSeparator);}

void SplitPath(StringView fullPath, StringView* oDirectoryPath,
	StringView* oNameOnly, StringView* oExtension, StringView* oName)
{
	index_t extLength = 0;
	StringView pathWithoutExt = Retro(Drop(Find(Retro(fullPath), '.', OptRef(extLength))));
	if(oExtension)
	{
		if(extLength<fullPath.Length()) *oExtension = fullPath.Tail(extLength);
		else *oExtension = null;
	}
	index_t nameLength = 0;
	StringView directoryWithSlash = Retro(Find(Retro(pathWithoutExt), IsPathSeparator, OptRef(nameLength)));
	if(oDirectoryPath) *oDirectoryPath = directoryWithSlash;
	if(oNameOnly) *oNameOnly = pathWithoutExt.Tail(nameLength);
	if(oName) *oName = fullPath.Tail(nameLength+1+extLength);
}

StringView ExtractDirectoryPath(StringView fullPath)
{
	StringView result;
	SplitPath(fullPath, &result, null, null, null);
	return result;
}

StringView ExtractNameWithoutExtension(StringView fullPath)
{
	StringView result;
	SplitPath(fullPath, null, &result, null, null);
	return result;
}

StringView ExtractName(StringView fullPath)
{
	StringView result;
	SplitPath(fullPath, null, null, null, &result);
	return result;
}

StringView ExtractExtension(StringView fullPath)
{
	StringView result;
	SplitPath(fullPath, null, null, &result, null);
	return result;
}

/*
String ReplaceExtension(StringView fullPath, StringView newExtension)
{
	StringView path, file;
	SplitPath(fullPath, &path, &file, null, null);
	return path + file + newExtension;
}
*/

StringView GetParentPath(StringView path)
{
	return Retro(Find(Retro(TrimRight(path, IsPathSeparator)), IsPathSeparator));
}

bool IsAbsolutePath(StringView path)
{
	if(path.Empty()) return false;
	if(IsPathSeparator(path.First())) return true;

#ifdef _WIN32
	if(path.Length() > 1 && path[0] >= 'A' && path[0] <= 'Z' && path[1] == ':')
		return true;
#endif

	return false;
}
INTRA_END
