#include "IO/ConsoleWriter.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#endif

namespace Intra { namespace IO {

void set_font(ConsoleTextWriter& s, const FontDesc& oldFont,
	Math::Vec3 color, float size, bool bold, bool italic, bool underline)
{
	(void)size;
	(void)italic;
#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
	(void)s;
	(void)bold;
	if(oldFont.Color!=color || oldFont.Underline!=underline)
	{
		ushort consoleCode = 0;
		if(color!=Math::Vec3(-1))
		{
			if(color.z>=0.25f) consoleCode |= FOREGROUND_BLUE;
			if(color.y>=0.25f) consoleCode |= FOREGROUND_GREEN;
			if(color.x>=0.25f) consoleCode |= FOREGROUND_RED;
			if(color.x>=0.5f || color.y>=0.5f || color.z>=0.5f)
				consoleCode |= FOREGROUND_INTENSITY;
		}
		else consoleCode |= FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED;
		if(underline) consoleCode |= COMMON_LVB_UNDERSCORE;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), consoleCode);
	}
#else
	if(oldFont.Color!=color || oldFont.Bold!=bold || oldFont.Underline!=underline)
	{
		s << "\x1B[0m";
		if(color!=Math::Vec3(-1))
		{
			int code = 0;
			int colorCode = 30;
			if(color.x>=0.25f) colorCode += 1;
			if(color.y>=0.25f) colorCode += 2;
			if(color.z>=0.25f) colorCode += 4;
			if(color.x<0.5f && color.y<0.5f && color.z<0.5f) code = 2;
			s << "\x1B[" << code << ';' << colorCode << 'm';
		}
		if(bold) s << "\x1B[1m";
		if(underline) s << "\x1B[4m";
	}
#endif
}

void ConsoleTextWriter::PushFont(Math::Vec3 color, float size, bool bold, bool italic, bool underline)
{
	auto oldFont = GetCurrentFont();
	mFontStack.AddLast({color, size, bold, italic, underline, false});
	set_font(*this, oldFont, color, size, bold, italic, underline);
}

void ConsoleTextWriter::PopFont()
{
	FontDesc oldFont = mFontStack.Last();
	mFontStack.RemoveLast();
	auto font = GetCurrentFont();
	set_font(*this, oldFont, font.Color, font.Size, font.Bold, font.Italic, font.Underline);
}

ConsoleTextWriter ConsoleWriter(&Console);

}}

INTRA_WARNING_POP
