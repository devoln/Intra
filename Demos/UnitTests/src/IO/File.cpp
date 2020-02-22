#include "IO/OsFile.h"
#include "IO/FileWriter.h"
#include "IO/FileReader.h"
#include "IO/FileSystem.h"
#include "IO/FormattedWriter.h"
#include "IO/HtmlWriter.h"

#include "Core/Range/Recurrence.h"
#include "Core/Range/ByLine.h"
#include "Core/Range/TakeByLine.h"
#include "Core/Range/Split.h"
#include "Core/Range/Map.h"
#include "Core/Range/Polymorphic/OutputRange.h"
#include "Core/Range/Polymorphic/InputRange.h"
#include "Core/Range/Stream/ToString.h"


#include "Utils/Endianess.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

using namespace Intra;

void TestFileSyncIO(FormattedWriter& output)
{
	auto file111 = OS.FileOpenOverwrite("TestFileSyncIO.txt", IgnoreErrors);
		file111.PrintLine("Fibonacci sequence: ", Take(Recurrence([](int a, int b) {return a + b;}, 1, 1), 10))
		.PrintLine("Closing file.");
	file111 = null;

	const String fileContents = OS.FileOpen("TestFileSyncIO.txt", IgnoreErrors);
	output.PrintLine("Written file contents:")
		.PrintLine(fileContents);

	INTRA_ASSERT_EQUALS(
		TakeByLine(fileContents).First(),
		"Fibonacci sequence: [1, 1, 2, 3, 5, 8, 13, 21, 34, 55]");

	INTRA_ASSERT_EQUALS(
		StringOf(Map(SplitLines(OS.FileOpen("TestFileSyncIO.txt", IgnoreErrors)), &String::Length)),
		"[54, 13]");


	char buf[100];
	INTRA_ASSERT_EQUALS(
		StringOfConsume(Map(OS.FileOpen("TestFileSyncIO.txt", IgnoreErrors).ByLine(buf), &StringView::Length)),
		"[54, 13]");

	char smallBuf[10];
	INTRA_ASSERT_EQUALS(
		StringOfConsume(Map(OS.FileOpen("TestFileSyncIO.txt", IgnoreErrors).ByLine(smallBuf), &StringView::Length)),
		"[10, 10, 10, 10, 10, 4, 10, 3]");

	size_t sumLength = 0;
	for(auto str: OS.FileOpen("TestFileSyncIO.txt", IgnoreErrors).ByLine(buf))
		sumLength += str.Length();
	INTRA_ASSERT_EQUALS(sumLength, 67);

	sumLength = 0;
	for(auto str: OS.FileOpen("TestFileSyncIO.txt", IgnoreErrors).ByLine(buf, Tags::KeepTerminator))
		sumLength += str.Length();
	INTRA_ASSERT_EQUALS(sumLength, 71);

	auto writer = HtmlWriter(OS.FileOpenAppend("TestFileSyncIO.txt", IgnoreErrors));
	writer.PushFont({0, 0.5f, 0}, 4, true);
	writer.PrintLine("Зелёный текст");
	writer.PopFont();
	writer = null;

	String newFileContents = OS.FileOpen("TestFileSyncIO.txt", IgnoreErrors);

	StringView htmlString = AtIndex(OS.FileOpen("TestFileSyncIO.txt", IgnoreErrors).ByLine(buf), 2);
	INTRA_ASSERT(StartsWith(htmlString, "<font color"));
	INTRA_ASSERT(Contains(htmlString, "Зелёный текст"));

	FileReader file = OS.FileOpen("TestFileSyncIO.txt", IgnoreErrors);
	uint32 value = file.RawRead<uint32LE>(); //Read little-endian uint32
	INTRA_ASSERT_EQUALS(char(value & 255), 'F');
	INTRA_ASSERT_EQUALS(char((value >> 8) & 255), 'i');
	INTRA_ASSERT_EQUALS(char((value >> 16) & 255), 'b');
	INTRA_ASSERT_EQUALS(char((value >> 24) & 255), 'o');

	value = OS.FileOpen("TestFileSyncIO.txt", IgnoreErrors).RawRead<uint32BE>(); //Read big-endian uint32
	INTRA_ASSERT_EQUALS(char(value & 255), 'o');
	INTRA_ASSERT_EQUALS(char((value >> 8) & 255), 'b');
	INTRA_ASSERT_EQUALS(char((value >> 16) & 255), 'i');
	INTRA_ASSERT_EQUALS(char((value >> 24) & 255), 'F');

	FileReader file2 = file;
	uint value1 = file.RawRead<uint32LE>();
	uint value2 = file2.RawRead<uint32LE>();
	INTRA_ASSERT_EQUALS(value1, value2);

	//Запись в стандартный вывод. Если это консоль, то на неё выведется цветной текст.
	output.PushFont({0, 0.5f, 0})
		.PrintLine("Зелёный текст")
		.PopFont();
}

void TestFileAsyncIO(FormattedWriter& output);
void TestFileSearching(FormattedWriter& output);


INTRA_WARNING_POP

