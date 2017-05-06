#include "FilePath.h"
#include "Range/Mutation/Replace.h"
#include "Range/Decorators/Retro.h"
#include "Range/Search/Trim.h"
#include "Utils/StringView.h"
#include "Utils/Span.h"
#include "Container/Sequential/String.h"
#include "Cpp/Warnings.h"
#include "Cpp/PlatformDetect.h"

namespace Intra { namespace IO { namespace Path {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void NormalizeSlashesAndSpaces(Span<char>& path)
{
	TrimAdvance(path, Op::IsSpace<char>);
	Replace(path, '\\', '/');
}

String AddTrailingSlash(StringView path)
{return (path.Last()=='/' || path.Last()=='\\')? String(path): path+'/';}

StringView RemoveTrailingSlash(StringView path)
{return TrimRight(path, IsPathSeparator);}

void SplitPath(StringView fullPath, StringView* oDirectoryPath,
	StringView* oNameOnly, StringView* oExtension, StringView* oName)
{
	size_t extLength = 0;
	StringView pathWithoutExt = Retro(Drop(Find(Retro(fullPath), '.', &extLength)));
	if(oExtension)
	{
		if(extLength<fullPath.Length()) *oExtension = fullPath.Tail(extLength);
		else *oExtension = null;
	}
	size_t nameLength = 0;
	StringView directoryWithSlash = Retro(Find(Retro(pathWithoutExt), IsPathSeparator, &nameLength));
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

String ReplaceExtension(StringView fullPath, StringView newExtension)
{
	StringView path, file;
	SplitPath(fullPath, &path, &file, null, null);
	return path+file+newExtension;
}

StringView GetParentPath(StringView path)
{
	using namespace Range;
	return Retro(Find(Retro(TrimRight(path, IsPathSeparator)), IsPathSeparator));
}

bool IsAbsolutePath(StringView path)
{
	if(path.Empty()) return false;
	if(IsPathSeparator(path.First())) return true;

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	if(path.Length()>1 && path[0]>='A' && path[0]<='Z' && path[1]==':')
		return true;
#endif

	return false;
}

INTRA_WARNING_POP

}}}
