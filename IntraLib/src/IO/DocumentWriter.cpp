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

static void set_console_font_color(Math::Vec3 color, bool underline)
{
#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
	ushort consoleCode = 0;
	if(color.z>=0.25f) consoleCode |= FOREGROUND_BLUE;
	if(color.y>=0.25f) consoleCode |= FOREGROUND_GREEN;
	if(color.x>=0.25f) consoleCode |= FOREGROUND_RED;
	if(color.x>=0.5f || color.y>=0.5f || color.z>=0.5f) consoleCode |= FOREGROUND_INTENSITY;
	if(underline) consoleCode |= COMMON_LVB_UNDERSCORE;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), consoleCode);
#endif
}

void ConsoleTextWriter::PushFont(Math::Vec3 color, float size)
{
	(void)size;
	set_console_font_color(color, underline);
	colorStack.AddLast(color);
}

void ConsoleTextWriter::PopFont()
{
	colorStack.RemoveLast();
	set_console_font_color(colorStack.Empty()? Math::Vec3(0.499f): colorStack.Last(), underline);
}

void ConsoleTextWriter::PushUnderline()
{
	underline = true;
	set_console_font_color(colorStack.Empty()? Math::Vec3(0.499f): colorStack.Last(), underline);
}

void ConsoleTextWriter::PopUnderline()
{
	underline = false;
	set_console_font_color(colorStack.Empty()? Math::Vec3(0.499f): colorStack.Last(), underline);
}

}}

