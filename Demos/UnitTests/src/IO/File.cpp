#include "IO/OsFile.h"
#include "IO/FileWriter.h"
#include "IO/FileReader.h"
#include "IO/FileSystem.h"
#include "IO/FormattedWriter.h"
#include "IO/HtmlWriter.h"
#include "Range/Generators/Recurrence.h"
#include "Range/Decorators/TakeByLine.h"
#include "Range/Decorators/Map.h"
#include "Platform/CppWarnings.h"
#include "Range/Iterator/RangeForSupport.h"
#include "Platform/Endianess.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

using namespace Intra;
using namespace IO;
using namespace Range;

void TestFileSyncIO(FormattedWriter& output)
{
	OS.FileOpenOverwrite("TestFileSyncIO.txt")
		.PrintLine("Fibonacci sequence: ", Take(Recurrence([](int a, int b) {return a+b;}, 1, 1), 10))
		.PrintLine("Closing file.");

	String fileContents = OS.FileOpen("TestFileSyncIO.txt");
	output.PrintLine("Written file contents:")
		.PrintLine(fileContents);

	INTRA_ASSERT_EQUALS(
		TakeByLine(fileContents).First(),
		"Fibonacci sequence: [1, 1, 2, 3, 5, 8, 13, 21, 34, 55]");

	INTRA_ASSERT_EQUALS(
		ToString(Map(ByLine(fileContents), &String::Length)),
		"[54, 13]");

	INTRA_ASSERT_EQUALS(
		ToString(Map(OS.FileOpen("TestFileSyncIO.txt").ByLine(), &String::Length)),
		"[54, 13]");


	char buf[100];
	INTRA_ASSERT_EQUALS(
		ToStringConsume(Map(OS.FileOpen("TestFileSyncIO.txt").ByLine(buf), &StringView::Length)),
		"[54, 13]");

	char smallBuf[10];
	INTRA_ASSERT_EQUALS(
		ToStringConsume(Map(OS.FileOpen("TestFileSyncIO.txt").ByLine(smallBuf), &StringView::Length)),
		"[10, 10, 10, 10, 10, 4, 10, 3]");

	size_t sumLength = 0;
	for(auto str: OS.FileOpen("TestFileSyncIO.txt").ByLine(buf))
		sumLength += str.Length();
	INTRA_ASSERT_EQUALS(sumLength, 67);

	sumLength = 0;
	for(auto str: OS.FileOpen("TestFileSyncIO.txt").ByLine(buf, Tags::KeepTerminator))
		sumLength += str.Length();
	INTRA_ASSERT_EQUALS(sumLength, 71);

	HtmlWriter(OS.FileOpenAppend("TestFileSyncIO.txt"))
		.PushFont({0, 0.5f, 0}, 4, true)
		.PrintLine("Зелёный текст")
		.PopFont();

	StringView htmlString = AtIndex(OS.FileOpen("TestFileSyncIO.txt").ByLine(buf), 2);
	INTRA_ASSERT(Algo::StartsWith(htmlString, "<font color"));
	INTRA_ASSERT(Algo::Contains(htmlString, "Зелёный текст"));

	FileReader file = OS.FileOpen("TestFileSyncIO.txt");
	uint value = file.ReadRaw<uintLE>();
	INTRA_ASSERT_EQUALS(char(value & 255), 'F');
	INTRA_ASSERT_EQUALS(char((value >> 8) & 255), 'i');
	INTRA_ASSERT_EQUALS(char((value >> 16) & 255), 'b');
	INTRA_ASSERT_EQUALS(char((value >> 24) & 255), 'o');

	value = OS.FileOpen("TestFileSyncIO.txt").ReadRaw<uintBE>(); //Читаем беззнаковое число в порядке байт big-endian
	INTRA_ASSERT_EQUALS(char(value & 255), 'o');
	INTRA_ASSERT_EQUALS(char((value >> 8) & 255), 'b');
	INTRA_ASSERT_EQUALS(char((value >> 16) & 255), 'i');
	INTRA_ASSERT_EQUALS(char((value >> 24) & 255), 'F');

	FileReader file2 = file;
	uint value1 = file.ReadRaw<uintLE>();
	uint value2 = file2.ReadRaw<uintLE>();
	INTRA_ASSERT_EQUALS(value1, value2);

	//Запись в стандартный вывод. Если это консоль, то на неё выведется цветной текст.
	output.PushFont({0, 0.5f, 0})
		.PrintLine("Зелёный текст")
		.PopFont();
}

void TestFileAsyncIO(FormattedWriter& output);
void TestFileSearching(FormattedWriter& output);


INTRA_WARNING_POP

