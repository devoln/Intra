#include "Intra/Core.h"
INTRA_PUSH_DISABLE_ALL_WARNINGS
#include "String.h"
#include "Intra/Misc/RawMemory.h"
#include "Extra/System/Debug.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_SIGN_COMPARE
template<typename Char> void TestShortStringOptimization()
{
	GenericString<Char> emptyStr;
	INTRA_ASSERT(emptyStr == "");
	INTRA_ASSERT(emptyStr == null);

	//always shorter than SSO buffer (even for char32_t on 32-bit platform)
	GenericString<Char> veryShortStr = "VS";
	INTRA_ASSERT(veryShortStr == "VS");
	INTRA_ASSERT(veryShortStr.Length() == Misc::CStringLength(veryShortStr.CStr())); //check null-terminator

	//fills 11-byte SSO buffer for char on 32-bit platforms, 12th byte must be the null-terminator
	GenericString<Char> shortStr = L"ShortString";
	INTRA_ASSERT(shortStr == "ShortString");
	INTRA_ASSERT(shortStr.Length() == Misc::CStringLength(shortStr.CStr())); //checks null-terminator

	//longer than SSO on 32-bit platforms, on 64-bit char version fits in SSO buffer
	GenericString<Char> longerStr = u"LongerString";
	INTRA_ASSERT("LongerString" == longerStr);
	INTRA_ASSERT(longerStr.Length() == Misc::CStringLength(longerStr.CStr()));

	//fills 23-byte SSO buffer for char on 64-bit platforms, 24th byte must be the null-terminator
	GenericString<Char> longStr = "This is a long string!!";
	INTRA_ASSERT(longStr == "This is a long string!!");
	INTRA_ASSERT(longStr.Length() == Misc::CStringLength(longStr.CStr()));

	//doesn't fit into SSO buffer on any platform, uses heap allocation
	GenericString<Char> veryLongStr = "This is a very long string!";
	INTRA_ASSERT(veryLongStr == "This is a very long string!");
	INTRA_ASSERT(veryLongStr.Length() == Misc::CStringLength(veryLongStr.CStr()));
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
INTRA_END
INTRA_WARNING_POP
