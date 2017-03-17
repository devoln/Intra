#include "IO/Console.h"
#include "Range/Special/Unicode.h"

#include <stdio.h>

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

ConsoleInOut::ConsoleInOut():
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	mReadBufCount(0), mOutputHandle(GetStdHandle(STD_OUTPUT_HANDLE))
#else
	mFirstInited(false)
#endif
{}

void ConsoleInOut::Put(char c)
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	mWriteBuf[mWriteBufCount++] = c;
	byte charLen = UTF8::SequenceBytes(mWriteBuf[0]);
	if(mWriteBufCount < charLen) return;

	wchar_t wbuf[2];
	int wsrcLength = MultiByteToWideChar(CP_UTF8, 0, mWriteBuf, charLen, wbuf, 2);
	DWORD written;
	WriteConsoleW(HANDLE(mOutputHandle), wbuf, DWORD(wsrcLength), &written, null);
	mWriteBufCount = 0;
#else
	write(STDOUT_FILENO, &c, 1);
#endif
}

size_t ConsoleInOut::CopyAdvanceFromAdvance(ArrayRange<const char>& src)
{
	if(src.Empty()) return 0;
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	const size_t totalBytesToWrite = src.Length();
	while(mWriteBufCount != 0)
	{
		Put(src.First());
		src.PopFirst();
		if(src.Empty()) return totalBytesToWrite;
	}

	const int wlen = MultiByteToWideChar(CP_UTF8, 0, src.Data(), src.Length(), null, 0);
	wchar_t wbuf[512];
	GenericString<wchar_t> wsrc;
	if(wlen>512) wsrc.SetCountUninitialized(size_t(wlen));
	auto wsrcPtr = wlen>512? wsrc.Data(): wbuf;
	int wsrcLength = MultiByteToWideChar(CP_UTF8, 0, src.Data(), src.Length(), wsrcPtr, wlen);
	DWORD written;
	WriteConsoleW(HANDLE(mOutputHandle), wsrcPtr, DWORD(wsrcLength), &written, null);
	src.Begin = src.End;

	return totalBytesToWrite;
#else
	size_t n = write(STDOUT_FILENO, src.Data(), src.Length());
	src.Begin += n;
	return n;
#endif
}

void ConsoleInOut::PopFirst()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	if(mReadBufCount!=0)
	{
		mReadBufCount--;
		mReadBufStart++;
		return;
	}
	DWORD read;
	wchar_t u16[2];
	ReadConsoleW(HANDLE(mInputHandle), u16, 1, &read, null);
	const bool isSurrogate = (u16[0]>=0xD800 && u16[0]<=0xDBFF);
	if(isSurrogate)
	{
		ReadConsoleW(HANDLE(mInputHandle), u16+1, 1, &read, null);
		read = 2;
	}
	int bytesPerChar = WideCharToMultiByte(CP_UTF8, 0, u16, int(read), mReadBuf, sizeof(mReadBuf), null, null);
	mReadBufStart = 0;
	mReadBufCount = byte(bytesPerChar);
#else
	read(STDIN_FILENO, &mFirst, 1);
	mFirstInited = true;
#endif
}

char ConsoleInOut::First() const
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	if(mReadBufCount == 0) const_cast<ConsoleInOut*>(this)->PopFirst();
	return mReadBuf[mReadBufStart];
#else
	if(!mFirstInited == 0) const_cast<ConsoleInOut*>(this)->PopFirst();
	return mFirst;
#endif
}

size_t ConsoleInOut::CopyAdvanceToAdvance(ArrayRange<char>& dst)
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
	size_t n = read(STDIN_FILENO, dst.Data(), dst.Length());
	dst.Begin += n;
	return n;
#endif
}

dchar ConsoleInOut::GetChar()
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
	read(STDIN_FILENO, c, 1);
	byte clen = UTF8::SequenceBytes(c[0]);
	read(STDIN_FILENO, c+1, size_t(clen-1u));
	dchar ch = UTF8(StringView(c, clen)).First();
	if(ch=='\r') ch = '\n';
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return dchar(ch);
#endif
}

void set_font(ConsoleInOut& s, const FontDesc& oldFont,
	Math::Vec3 color, float size, bool bold, bool italic, bool underline)
{

}

void ConsoleInOut::pushFont(const FontDesc& newFont)
{
	FontDesc oldFont;
	if(!mFontStack.Empty()) oldFont = mFontStack.Last();
	else
	{
		oldFont = FontDesc::Default();
		oldFont.Color = {-1, -1, -1};
	}
#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
	if(oldFont.Color != newFont.Color ||
		oldFont.Underline != newFont.Underline)
	{
		ushort consoleCode = 0;
		if(newFont.Color!=Math::Vec3(-1))
		{
			if(newFont.Color.z>=0.25f) consoleCode |= FOREGROUND_BLUE;
			if(newFont.Color.y>=0.25f) consoleCode |= FOREGROUND_GREEN;
			if(newFont.Color.x>=0.25f) consoleCode |= FOREGROUND_RED;
			if(newFont.Color.x>=0.5f || newFont.Color.y>=0.5f || newFont.Color.z>=0.5f)
				consoleCode |= FOREGROUND_INTENSITY;
		}
		else consoleCode |= FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED;
		if(newFont.Underline) consoleCode |= COMMON_LVB_UNDERSCORE;
		SetConsoleTextAttribute(HANDLE(mOutputHandle), consoleCode);
	}
#else
	if(oldFont.Color != newFont.Color ||
		oldFont.Bold != newFont.Bold ||
		oldFont.Underline != newFont.Underline)
	{
		Print("\x1B[0m");
		if(newFont.Color != Math::Vec3(-1))
		{
			int code = 0;
			int colorCode = 30;
			if(newFont.Color.x>=0.25f) colorCode += 1;
			if(newFont.Color.y>=0.25f) colorCode += 2;
			if(newFont.Color.z>=0.25f) colorCode += 4;
			if(newFont.Color.x<0.5f && newFont.Color.y<0.5f && newFont.Color.z<0.5f) code = 2;
			Print("\x1B[", code, ';', colorCode, 'm');
		}
		if(newFont.Bold) Print("\x1B[1m");
		if(newFont.Underline) Print("\x1B[4m");
	}
#endif
}

void ConsoleInOut::popFont()
{
	FontDesc prevFont;
	if(mFontStack.Length() >= 2) prevFont = mFontStack[mFontStack.Length()-2];
	else
	{
		prevFont = FontDesc::Default();
		prevFont.Color = {-1, -1, -1};
	}
	pushFont(prevFont);
}

ConsoleInOut Console;

}}
