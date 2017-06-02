#include "Data/Serialization.hh"
#include "IO/FormattedWriter.h"
#include "Data/Reflection.h"

using namespace Intra;
using namespace IO;

struct StructTest
{
	int x;
	float y;
	String z[2];
	Array<int> arr;

	INTRA_ADD_REFLECTION(StructTest, x, y, z, arr)
};

void TestTextSerialization(FormattedWriter& output)
{
	StringView strToDeserialize = "{z = [\"serialization\", \"test\"], x = ---5434, "
		"arr = [1, 2, 3, -4, 5], y = 2.1721}";
	output.PrintLine("In custom format: ");
	output.PrintLine(strToDeserialize);
	output.LineBreak();
	auto deserializer = Data::TextDeserializer(Data::LanguageParams::JsonLikeNoQuotes, strToDeserialize);
	StructTest t2 = deserializer.Deserialize<StructTest>();
	INTRA_ASSERT_EQUALS(deserializer.Log, null);
	if(!deserializer.Log.Empty())
	{
		output.PrintLine("Deserialization Log: ");
		output.PrintLine(deserializer.Log);
	}

	char serializedBuf[300];
	Data::TextSerializer serializer(Data::LanguageParams::Json,
		Data::TextSerializerParams::Verbose, serializedBuf);
	serializer << t2;
	const StringView resultJson = serializer.Output.GetWrittenData();
	const StringView expectedResultJson =
	"{\r\n"
		"\t\"x\" : -5434,\r\n"
		"\t\"y\" : 2.1721,\r\n"
		"\t\"z\" : [\"serialization\", \"test\"],\r\n"
		"\t\"arr\" : [1, 2, 3, -4, 5]\r\n"
	"}";
	INTRA_ASSERT_EQUALS(resultJson, expectedResultJson);
	(void)expectedResultJson;
	output.PrintLine("JSON:");
	output.PrintLine(resultJson);
	output.LineBreak();

	serializer = Data::TextSerializer(Data::LanguageParams::Xml,
		Data::TextSerializerParams::VerboseNoSpaces, serializedBuf);
	serializer << t2;
	const StringView resultXml = serializer.Output.GetWrittenData();
	output.PrintLine("XML:");
	output.PrintLine(resultXml);
	output.LineBreak();

	serializer = Data::TextSerializer(Data::LanguageParams::JsonLikeNoQuotes,
		Data::TextSerializerParams::Verbose, serializedBuf);
	serializer << t2;
	const StringView resultJsonLikeNoQuotes = serializer.Output.GetWrittenData();
	const StringView expectedResultJsonLikeNoQuotes =
	"{\r\n"
		"\tx = -5434,\r\n"
		"\ty = 2.1721,\r\n"
		"\tz = [\"serialization\", \"test\"],\r\n"
		"\tarr = [1, 2, 3, -4, 5]\r\n"
	"}";
	output.PrintLine("JSON-like without quotes:");
	output.PrintLine(resultJsonLikeNoQuotes);
	INTRA_ASSERT_EQUALS(resultJsonLikeNoQuotes, expectedResultJsonLikeNoQuotes);
	(void)expectedResultJsonLikeNoQuotes;
}

void TestBinarySerialization(IO::FormattedWriter& output)
{
	byte data[1000];
	Data::BinarySerializer serializer(data);
	output.PrintLine("Binary serialization to array byte data[1000]:");
	
	int origA = 12345;
	serializer << origA;
	output.PrintLine("int origA = ", origA);

	float origB = 3.1415926f;
	serializer << origB;
	output.PrintLine("float originalB = ", origB);
	
	StringView origC = "C";
	serializer << origC;
	output.PrintLine("StringView origC = ", origC);
	
	String origD = "D";
	serializer << origD;
	output.PrintLine("String origD = ", origD);
	
	int origE[] = {3234, 23, 423, 423, 22};
	serializer << origE;
	output.PrintLine("int origE[5] = ", origE);
	
	CSpan<int> origF = origE;
	serializer << origF;
	output.PrintLine("CSpan<int> originalF = ", origF);
	
	Array<int> origG = origF;
	serializer << origG;
	output.PrintLine("Array<int> origG = ", origG);

	Meta::Tuple<int, double> origH = {-8543211, 2.718281828};
	serializer << origH;
	output.PrintLine("Tuple<int, double> origH = ", origH);


	output.PrintLine("Deserializing from binary serializer output in data byte array:");

	Data::BinaryDeserializer deserializer(serializer.Output.GetWrittenData());
	int resA;
	deserializer >> resA;
	output.PrintLine("int resA = ", resA);
	INTRA_ASSERT_EQUALS(origA, resA);

	float resB = deserializer.Deserialize<float>();
	output.PrintLine("float resB = ", resB);
	INTRA_ASSERT_EQUALS(origB, resB);

	StringView resC = deserializer.Deserialize<StringView>();
	output.PrintLine("StringView resC = ", resC);
	INTRA_ASSERT_EQUALS(origC, resC);

	String resD = deserializer.Deserialize<String>();
	output.PrintLine("String resD = ", resD);
	INTRA_ASSERT_EQUALS(origD, resD);

	int resE[5];
	deserializer >> resE;
	output.PrintLine("int resE[5] = ", resE);
	INTRA_ASSERT2(Range::Equals(origE, resE), origE, resE);

	CSpan<int> resF = deserializer.Deserialize<CSpan<int>>();
	output.PrintLine("CSpan<int> resF = ", resF);
	INTRA_ASSERT2(Equals(origF, resF), origF, resF);

	Array<int> resG = deserializer.Deserialize<Array<int>>();
	output.PrintLine("Array<int> resG = ", resG);
	INTRA_ASSERT_EQUALS(origG, resG);

	Meta::Tuple<int, double> resH = deserializer.Deserialize<Meta::Tuple<int, double>>();
	output.PrintLine("Tuple<int, double> resH = ", resH);
	INTRA_ASSERT_EQUALS(origH, resH);
}
