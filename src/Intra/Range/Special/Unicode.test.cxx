#include "Intra/Range/Special/Unicode.h"
#include "IntraX/IO/FormattedWriter.h"
#include "Intra/Assert.h"

using namespace Intra;

void TestUnicodeConversion(FormattedWriter& output)
{
	StringView originalStr = "中国人 日本人 Русский ελληνικά αβγδεζηθικλμνξο Français العربية";
	//UTF8 -> UTF16 -> UTF32 -> UTF16 -> UTF8
	GenericString<char16_t> utf16 = ToUtf16(ToUtf32(originalStr));
	GenericString<char32_t> utf32 = ToUtf32(utf16);
	utf16 = ToUtf16(utf32);
	String utf8 = ToUtf8(ToUtf32(utf16.View()));
	INTRA_ASSERT_EQUALS(originalStr, utf8);
}
