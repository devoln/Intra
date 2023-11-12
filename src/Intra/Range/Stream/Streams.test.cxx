#include "Intra/Range.h"
#include "Intra/Range/Stream/Parse.h"
#include "Intra/Range/StringView.h"
#include "IntraX/Container/Sequential/List.h"
#include "IntraX/IO/FormattedWriter.h"
#include "IntraX/Unstable/Random/FastUniform.h"

namespace Intra { INTRA_BEGIN

INTRA_MODULE_UNITTEST
{
	char bufOnStack[512];
	Array<int> arrToFormat = {54, 3, 45, 56, 24};
	int rawArr[] = {6, 3, 8, 3};
	SpanOutput<char> buf = bufOnStack;
	buf << "Any output range with Put(char) method is a stream" << '!';
	INTRA_ASSERT(Equals(
		buf.WrittenRange(),
		"Any output range with Put(char) method is a stream!"
	));
	buf.Reset();
	INTRA_ASSERT(Abs(1.0f+Parse<float>("3.1415926") - 4.1415926f) < 0.00000001f);
	buf << arrToFormat;
	INTRA_ASSERT(Equals(
		buf.WrittenRange(),
		"[54, 3, 45, 56, 24]"
	));
	buf.Reset();
	static_assert(CConsumableRange<TRangeOfRef<Array<int>&>>);
	buf << Map(arrToFormat, &Sqr<int>);
	INTRA_ASSERT(Equals(
		buf.WrittenRange(),
		"[2916, 9, 2025, 3136, 576]"
	));
	buf.Reset();
	buf << Map(rawArr, &Sqr<int>);
	INTRA_ASSERT(Equals(
		buf.WrittenRange(),
		"[36, 9, 64, 9]"
	));

	StringView strPiE = "3.1415926, 2.178281828, 1, 2";
	float pi, e;
	int i1, i2;
	strPiE >> pi >> "," >> e >> "," >> i1 >> "," >> i2;
	INTRA_ASSERT1(Abs(pi - 3.1415926f) < 0.00000001f, pi);
	INTRA_ASSERT1(Abs(e - 2.1782818f) < 0.00000001f, e);
	INTRA_ASSERT_EQUALS(i1, 1);
	INTRA_ASSERT1(i2, 2);

	INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
	struct OutputCaesarCipher
	{
		void Put(char ch)
		{
			if(ch >= 'A' && ch <= 'Z')
			{
				if(ch == 'Z') ch = 'A';
				else ch++;
			}
			if(ch >= 'a' && ch <= 'z')
			{
				if(ch == 'z') ch = 'a';
				else ch++;
			}
			Output << ch;
		}
		OutputStream Output;
	};

	StringView strings[] = {"Hello", ", ", "World", "!"};
	String dst;
	OutputCaesarCipher{LastAppender(dst)} << "Printing array: " << strings;
	INTRA_ASSERT_EQUALS(dst, "TODO");
}

} INTRA_END
