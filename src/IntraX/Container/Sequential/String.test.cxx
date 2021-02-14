#include "Intra/Core.h"
INTRA_PUSH_DISABLE_ALL_WARNINGS
#include "String.h"
#include "Intra/Misc/RawMemory.h"
#include "IntraX/System/Debug.h"

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_SIGN_COMPARE
template<typename Char> void TestShortStringOptimization()
{
	GenericString<Char> emptyStr;
	INTRA_ASSERT(emptyStr == "");
	INTRA_ASSERT(emptyStr.Empty());

	//always shorter than SSO buffer (even for char32_t on 32-bit platform)
	GenericString<Char> veryShortStr = "VS";
	INTRA_ASSERT(veryShortStr == "VS");
	INTRA_ASSERT(veryShortStr.Length() == StringView(veryShortStr.CStr()).RawUnicodeUnits().Length()); //check nullptr-terminator

	//fills 11-byte SSO buffer for char on 32-bit platforms, 12th byte must be the nullptr-terminator
	GenericString<Char> shortStr = L"ShortString";
	INTRA_ASSERT(shortStr == "ShortString");
	INTRA_ASSERT(shortStr.Length() == StringView(shortStr.CStr()).RawUnicodeUnits().Length()); //checks nullptr-terminator

	//longer than SSO on 32-bit platforms, on 64-bit char version fits in SSO buffer
	GenericString<Char> longerStr = u"LongerString";
	INTRA_ASSERT("LongerString" == longerStr);
	INTRA_ASSERT(longerStr.Length() == StringView(longerStr.CStr()).RawUnicodeUnits().Length());

	//fills 23-byte SSO buffer for char on 64-bit platforms, 24th byte must be the nullptr-terminator
	GenericString<Char> longStr = "This is a long string!!";
	INTRA_ASSERT(longStr == "This is a long string!!");
	INTRA_ASSERT(longStr.Length() == StringView(longStr.CStr()).RawUnicodeUnits().Length());

	//doesn't fit into SSO buffer on any platform, uses heap allocation
	GenericString<Char> veryLongStr = "This is a very long string!";
	INTRA_ASSERT(veryLongStr == "This is a very long string!");
	INTRA_ASSERT(veryLongStr.Length() == StringView(veryLongStr.CStr()).RawUnicodeUnits().Length());
}

INTRA_MODULE_UNITTEST
{
	TestShortStringOptimization<char>();
	TestShortStringOptimization<char16_t>();
	TestShortStringOptimization<char32_t>();
	TestShortStringOptimization<wchar_t>();
#ifdef __cpp_char8_t
	TestShortStringOptimization<char8_t>();
#endif
}
} INTRA_END
INTRA_WARNING_POP
