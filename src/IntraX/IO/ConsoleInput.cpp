#include "IntraX/IO/ConsoleInput.h"
#include "Intra/Range/Special/Unicode.h"

INTRA_PUSH_DISABLE_ALL_WARNINGS
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif
#include <io.h>
#include <Windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
INTRA_WARNING_POP

INTRA_BEGIN
#ifdef _WIN32
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
#ifdef _WIN32
	if(gReadBufCount != 0)
	{
		gReadBufCount--;
		gReadBufStart++;
		return;
	}
	DWORD read;
	wchar_t u16[2];
	ReadConsoleW(gInputHandle, u16, 1, &read, null);
	const bool isSurrogate = (u16[0] >= 0xD800 && u16[0] <= 0xDBFF);
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
#ifdef _WIN32
	if(gReadBufCount == 0) const_cast<ConsoleInput*>(this)->PopFirst();
	return gReadBuf[gReadBufStart];
#else
	if(!gFirstInited) const_cast<ConsoleInput*>(this)->PopFirst();
	return gFirst;
#endif
}

index_t ConsoleInput::ReadWrite(Span<char>& dst)
{
#ifdef _WIN32
	const auto dstLen = dst.Length();
	while(!dst.Empty())
	{
		dst.Put(First());
		PopFirst();
	}
	return dstLen;
#else
	const auto n = index_t(read(STDIN_FILENO, dst.Data(), dst.Length()));
	dst.Begin += n;
	return n;
#endif
}

char32_t ConsoleInput::GetChar()
{
#ifdef _WIN32
	auto ch = _getwch();
	return ch=='\r'? char32_t('\n'): char32_t(ch);
#else
	termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~uint32(ICANON|ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	char c[6];
	const size_t charsRead = size_t(read(STDIN_FILENO, c, 1));
	size_t clen = UTF8::SequenceBytes(c[0]);
	clen = charsRead+size_t( read(STDIN_FILENO, c+1, clen-1) );
	char32_t ch = UTF8(StringView(c, clen)).First();
	if(ch=='\r') ch = '\n';
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return char32_t(ch);
#endif
}

INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
ConsoleInput ConsoleIn;
INTRA_END
