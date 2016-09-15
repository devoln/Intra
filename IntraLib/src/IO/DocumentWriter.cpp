#include "IO/DocumentWriter.h"

#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace Intra { namespace IO {

//Это нужно вставить в начало html файла, если используются спойлеры
const char* const HtmlWriter::CssSpoilerCode = "<style type=\"text/css\">"
	".spoiler > input +.box > blockquote{display: none;}"
	".spoiler > input:checked +.box > blockquote{display: block;}"
	".spoiler > input[type='checkbox'] {cursor: pointer; border-color:transparent!important; border-style:none!important;"
	"background:transparent none!important; position:relative; z-index:1; margin:-10px 0 -30px -230px;}"
	".spoiler span.close,"
	".spoiler span.open{padding-left:22px; color: #00f!important; text-decoration: underline; }"
	".spoiler > input +.box > span.close{display: none; }"
	".spoiler > input:checked +.box > span.close{display: inline; }"
	".spoiler > input:checked +.box > span.open{display: none; }"
	".spoiler > input +.box > span.open{display: inline; }"
	".spoiler blockquote,"
	".spoiler{padding:1em; border-radius:15px; -webkit-border-radius:15px;"
	"-khtml-border-radius:15px; -moz-border-radius:15px;"
	"-o-border-radius:15px; -ms-border-radius:15px; }"
	".spoiler{overflow-x:hidden; box-shadow: 0px 3px 8px #808080; border:#E5E5E5 solid 2px;"
	"-webkit-box-shadow:0px 3px 8px #808080; -khtml-box-shadow:0px 3px 8px #808080;"
	"-moz-box-shadow:0px 3px 8px #808080; -ms-box-shadow:0px 3px 8px #808080; }"
	"</style>";

const char* const HtmlWriter::CssLoggerCode = "<style type=\"text/css\">"
	".error {color: #ff0000; font-size: 125%;}\r\n"
	".warn {color: #ffaa00; font-size: 110%;}\r\n"
	".info {color: #707070; font-size: 100%;}\r\n"
	".normal {color: #000000; font-size: 100%;}\r\n"
	".perf {color: #aaaa00; font-size: 75%;}\r\n"
	"</style>";


void HtmlWriter::PushFont(Math::Vec3 color, float size, bool bold, bool italic, bool underline)
{
	auto curFont = GetCurrentFont();
	if(color==Math::NaN) color = {0,0,0};
	if(size==Math::NaN) size=3;
	font_stack.AddLast({color, size, bold, italic, underline, false});
	if(curFont.Underline && !underline) RawPrint("</ins>");
	if(curFont.Italic && !italic) RawPrint("</i>");
	if(curFont.Bold && !bold) RawPrint("</b>");
	if(curFont.Color!=color || curFont.Size!=size)
	{
		const auto c = Math::USVec3(Math::Min(color*255.0f, 255.0f));
		String colstr = String::Format()((c.x<<16)|(c.y<<8)|c.z, 6, '0', 16);
		RawPrint("<font color="+colstr+" size="+ToString(size)+">");
	}
	if(!curFont.Bold && bold) RawPrint("<b>");
	if(!curFont.Italic && italic) RawPrint("<i>");
	if(!curFont.Underline && underline) RawPrint("<ins>");
}

void HtmlWriter::PopFont()
{
	auto oldFont = GetCurrentFont();
	font_stack.RemoveLast();
	auto newFont = GetCurrentFont();
	if(oldFont.Bold && !newFont.Bold) RawPrint("</b>");
	if(oldFont.Italic && !newFont.Italic) RawPrint("</i>");
	if(oldFont.Underline && !newFont.Underline) RawPrint("</ins>");
	RawPrint("</font>");
	if(!oldFont.Bold && newFont.Bold) RawPrint("<b>");
	if(!oldFont.Italic && newFont.Italic) RawPrint("<i>");
	if(!oldFont.Underline && newFont.Underline) RawPrint("<ins>");
}

void set_font(ConsoleTextWriter& s, const FontDesc& oldFont, Math::Vec3 color, float size, bool bold, bool italic, bool underline)
{
	if(color==Math::NaN) color=Math::Vec3(0.499f);
	(void)size;
	(void)italic;
#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
	(void)s;
	(void)bold;
	if(oldFont.Color!=color || oldFont.Underline!=underline)
	{
		ushort consoleCode = 0;
		if(color!=Math::NaN)
		{
			if(color.z>=0.25f) consoleCode |= FOREGROUND_BLUE;
			if(color.y>=0.25f) consoleCode |= FOREGROUND_GREEN;
			if(color.x>=0.25f) consoleCode |= FOREGROUND_RED;
			if(color.x>=0.5f || color.y>=0.5f || color.z>=0.5f) consoleCode |= FOREGROUND_INTENSITY;
		}
		else
		{
			consoleCode |= FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED;
		}
		if(underline) consoleCode |= COMMON_LVB_UNDERSCORE;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), consoleCode);
	}
#else
	color*=2.0f;
	if(oldFont.Color!=color || oldFont.Bold!=bold || oldFont.Underline!=underline)
	{
		s << "\x1B[0m";
		s << "\x1B[0m";
		s << "\x1B[0;30m";
		int code = 0;
		int colorCode = 30;
		if(color.x>=0.25f) colorCode += 1;
		if(color.y>=0.25f) colorCode += 2;
		if(color.z>=0.25f) colorCode += 4;
		if(color.x<0.5f && color.y<0.5f && color.z<0.5f) code = 2;
		s << "\x1B[" << code << ';' << colorCode << 'm';
		if(bold) s << "\x1B[1m";
		if(underline) s << "\x1B[4m";
	}
#endif
}

void ConsoleTextWriter::PushFont(Math::Vec3 color, float size, bool bold, bool italic, bool underline)
{
	auto oldFont = GetCurrentFont();
	font_stack.AddLast({color, size, bold, italic, underline, false});
	set_font(*this, oldFont, color, size, bold, italic, underline);
}

void ConsoleTextWriter::PopFont()
{
	FontDesc oldFont = font_stack.Last();
	font_stack.RemoveLast();
	auto font = GetCurrentFont();
	set_font(*this, oldFont, font.Color, font.Size, font.Bold, font.Italic, font.Underline);
}

}}

