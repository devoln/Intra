#include "IO/ConsoleInput.h"
#include "Range/Special/Unicode.h"
#include "Platform/PlatformInfo.h"
#include "Platform/CppWarnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif

#include <io.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>
#include <conio.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else
#include <termios.h>
#include <unistd.h>
#endif

namespace Intra { namespace IO {

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
static byte gReadBufStart = 0;
static byte gReadBufCount = 0;
static char gReadBuf[6];
static HANDLE gInputHandle;
#else
static char gFirst;
static byte gFirstInited;
#endif

void ConsoleInput::PopFirst()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	if(gReadBufCount!=0)
	{
		gReadBufCount--;
		gReadBufStart++;
		return;
	}
	DWORD read;
	wchar_t u16[2];
	ReadConsoleW(gInputHandle, u16, 1, &read, null);
	const bool isSurrogate = (u16[0]>=0xD800 && u16[0]<=0xDBFF);
	if(isSurrogate)
	{
		ReadConsoleW(gInputHandle, u16+1, 1, &read, null);
		read = 2;
	}
	int bytesPerChar = WideCharToMultiByte(CP_UTF8, 0, u16, int(read), gReadBuf, sizeof(gReadBuf), null, null);
	gReadBufStart = 0;
	gReadBufCount = byte(bytesPerChar);
#else
	gFirstInited = read(STDIN_FILENO, &gFirst, 1) == 1;
#endif
}

char ConsoleInput::First() const
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	if(gReadBufCount == 0) const_cast<ConsoleInput*>(this)->PopFirst();
	return gReadBuf[gReadBufStart];
#else
	if(!gFirstInited) const_cast<ConsoleInput*>(this)->PopFirst();
	return gFirst;
#endif
}

size_t ConsoleInput::CopyAdvanceToAdvance(ArrayRange<char>& dst)
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	const size_t dstLen = dst.Length();
	while(!dst.Empty())
	{
		dst.Put(First());
		PopFirst();
	}
	return dstLen;
#else
	size_t n = size_t(read(STDIN_FILENO, dst.Data(), dst.Length()));
	dst.Begin += n;
	return n;
#endif
}

dchar ConsoleInput::GetChar()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	auto ch = _getwch();
	return ch=='\r'? dchar('\n'): dchar(ch);
#else
	termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~uint(ICANON|ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	char c[6];
	const size_t charsRead = size_t(read(STDIN_FILENO, c, 1));
	size_t clen = UTF8::SequenceBytes(c[0]);
	clen = charsRead+size_t( read(STDIN_FILENO, c+1, clen-1) );
	dchar ch = UTF8(StringView(c, clen)).First();
	if(ch=='\r') ch = '\n';
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return dchar(ch);
#endif
}

ConsoleInput ConsoleIn;

}}

INTRA_WARNING_POP
