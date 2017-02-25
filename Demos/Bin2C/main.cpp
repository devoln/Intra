#include "IO/Stream.h"
#include "IO/File.h"
#include "Range/Generators/StringView.h"
#include "Container/Sequential/String.h"
#include "Algo/String/Parse.h"
#include "Algo/String/Path.h"
#include "Algo/Search/Trim.h"
#include "Algo/Mutation/Transform.h"
using namespace Intra;
using namespace Intra::IO;

INTRA_DISABLE_REDUNDANT_WARNINGS

static String MakeIdentifierFromPath(StringView path, StringView prefix=null)
{
	String result = Algo::Path::ExtractName(path);
	Algo::Transform(result.AsRange(), [](char c){return AsciiSet::LatinAndDigits.Contains(c)? c: '_';});
	result = prefix+result;
	if(byte(result[0]-'0')<=9) result = '_'+result;
	return result;
}

StringView PathFromCmdLine(const char* path)
{
	return Algo::Trim(Algo::Trim(StringView(path), '"'), Op::IsSpace<char>);
}

static void ParseCommandLine(int argc, const char* argv[], String& inputFilePath, String& outputFilePath,
	String& binArrName, int& valuesPerLine, bool& notabs, bool& nospaces, bool& singleline, bool& endline)
{
	inputFilePath = null;
	outputFilePath = null;
	binArrName = null;
	valuesPerLine = 0;
	notabs = false;
	nospaces = false;
	singleline = false;
	endline = false;

	for(int i=1; i<argc; i++)
	{
		if(argv[i][0]!='-')
		{
			inputFilePath = PathFromCmdLine(argv[i]);
			continue;
		}

		if(StringView(argv[i])=="-notabs")
		{
			notabs = true;
			continue;
		}
		if(StringView(argv[i])=="-nospaces")
		{
			nospaces = true;
			continue;
		}
		if(StringView(argv[i])=="-singleline")
		{
			singleline = true;
			continue;
		}
		if(StringView(argv[i])=="-endline")
		{
			endline = true;
			continue;
		}

		if(StringView(argv[i])=="-o")
		{
			outputFilePath = PathFromCmdLine(argv[++i]);
			continue;
		}
		if(StringView(argv[i])=="-varname")
		{
			binArrName = PathFromCmdLine(argv[++i]);
			continue;
		}
		if(StringView(argv[i])=="-per-line")
		{
			StringView(argv[++i]) >> valuesPerLine;
			continue;
		}
	}

	if(outputFilePath==null) outputFilePath = inputFilePath+".c";
	if(binArrName==null) binArrName = MakeIdentifierFromPath(inputFilePath);
	if(valuesPerLine==0) valuesPerLine = 32;
}



inline size_t ByteToStr(byte x, char* dst)
{
	size_t len = 0;
	if(x>=100) dst[len++] = char('0'+x/100);
	if(x>=10) dst[len++] = char('0'+x/10%10);
	dst[len++] = char('0'+x%10);
	return len;
}


void ConvertFile(StringView inputFilePath, StringView outputFilePath, StringView binArrName,
	int valuesPerLine, bool notabs, bool nospaces, bool singleline, bool endline)
{
	DiskFile::Reader input(inputFilePath);
	if(input==null)
	{
		Console.PrintLine("File ", inputFilePath, " is not opened!");
		return;
	}
	ArrayRange<const byte> src = input.Map<byte>();
	const byte* srcBytes = src.Begin;
	DiskFile::Writer dst(outputFilePath);
	dst.Print("const unsigned char ", binArrName, "[] = {");
	if(!singleline)
	{
		dst.PrintLine();
		if(!notabs) dst.Print('\t');
	}
	char dataToPrint[10]={',', ' '};
	static const char dataToPrintPerLine[4]="\r\n\t";
	const size_t sizeToPrintPerLineCount = 3u-size_t(notabs);
	size_t sizeToPrint = 2u-size_t(nospaces);

	if(src!=null) dst << int(srcBytes[0]);
	if(singleline) for(size_t i=1; i<src.Length(); i++)
	{
		size_t n = sizeToPrint;
		n += ByteToStr(srcBytes[i], dataToPrint+n);
		dst.WriteData(dataToPrint, n);
	}
	else for(size_t i=1; i<src.Length(); i++)
	{
		size_t n = sizeToPrint;
		if(!singleline && i%size_t(valuesPerLine)==0)
		{
			C::memcpy(dataToPrint+n, dataToPrintPerLine, sizeToPrintPerLineCount);
			n += sizeToPrintPerLineCount;
		}
		n += ByteToStr(srcBytes[i], dataToPrint+n);
		dst.WriteData(dataToPrint, n);
	}
	if(!singleline) dst.PrintLine();
	dst.Print("};");
	if(endline) dst.PrintLine();
}



int INTRA_CRTDECL main(int argc, const char* argv[])
{
	if(argc<2)
	{
		Console.PrintLine("Usage:");
		Console.PrintLine(argv[0], " <inputFilePath> [-o <outputFilePath.c>] [-varname <binaryArrayName>] [-per-line <32>] [-notabs] [-nospaces] [-singleline] [-endline]");
		return 0;
	}

	String inputFilePath, outputFilePath, binArrName;
	int valuesPerLine;
	bool notabs, nospaces, singleline, endline;
	ParseCommandLine(argc, argv, inputFilePath, outputFilePath, binArrName, valuesPerLine, notabs, nospaces, singleline, endline);
	ConvertFile(inputFilePath, outputFilePath, binArrName, valuesPerLine, notabs, nospaces, singleline, endline);

	return 0;
}
