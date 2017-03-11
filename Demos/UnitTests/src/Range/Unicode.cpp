#include "Range/Special/Unicode.h"
#include "IO/FormattedWriter.h"
#include "Platform/Debug.h"

using namespace Intra;
using namespace Range;

void TestUnicodeConversion(IO::IFormattedWriter& output)
{
	output.PrintLine("String to test:");
	StringView originalStr = "Тестируется русский текст с иероглифами ㈇㌤㈳㌛㉨, "
		"а также греческим алфавитом αβγδεζηθικλμνξο.";
	output.PrintLine(originalStr);
	output.PrintLine("UTF8 -> UTF16 -> UTF32 -> UTF16 -> UTF8");
	String mystr = originalStr;
	WString utf16 = UTF8(originalStr).ToUTF16(false);
	DString utf32 = UTF16(utf16).ToUTF32();
	utf16 = UTF32(utf32).ToUTF16(false);
	auto utf8 = UTF16(utf16.View()).ToUTF8();
	output.PrintLine("Result:");
	output.PrintLine(originalStr);
	INTRA_ASSERT_EQUALS(originalStr, utf8);
}
