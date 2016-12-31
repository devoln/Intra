#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include "PerfTestSerialization.h"

#include "Meta/Preprocessor.h"
#include "Data/BinarySerialization.h"
#include "Data/TextSerialization.h"
#include "Data/Reflection.h"
#include "Platform/Time.h"
#include "Range/ArrayRange.h"
#include "IO/Stream.h"
#include "Test/PerformanceTest.h"

using namespace Intra;
using namespace Intra::IO;

/*struct StructTest
{
	int x;
	float y;
	StringView z[2];
	Array<int> arr;
	INTRA_ADD_REFLECTION(StructTest, x, y, z, arr);
};

int main()
{
	StringView strToDeserialize = "{z = [\"тест\", \"сериализации\"], x = ---5434, arr = [1, 2, 3, -4, 5], y = 2.1721}";
	Console.PrintLine("In custom format: ", endl, strToDeserialize, endl);
	Data::TextDeserializer deserializer(Data::DataLanguageParams::JsonLikeNoQuotes, strToDeserialize);
	StructTest t2 = deserializer.Deserialize<StructTest>();
	if(!deserializer.Log.Empty()) Console.PrintLine("Deserialization Log: ", endl, deserializer.Log);

	char serializedBuf[100];
	Data::TextSerializer serializer(Data::DataLanguageParams::Json,
		Data::TextSerializerParams::Verbose, ArrayRange<char>(serializedBuf));
	serializer.Serialize(t2);
	Console.PrintLine("JSON:", endl, serializer.GetString(), endl);

	serializer = Data::TextSerializer(Data::DataLanguageParams::Xml,
		Data::TextSerializerParams::VerboseNoSpaces, ArrayRange<char>(serializedBuf));
	serializer.Serialize(t2);
	Console.PrintLine("XML:", endl, serializer.GetString(), endl);

	serializer = Data::TextSerializer(Data::DataLanguageParams::JsonLikeNoQuotes,
		Data::TextSerializerParams::Verbose, ArrayRange<char>(serializedBuf));
	serializer.Serialize(t2);
	Console.PrintLine("JSON-like without quotes:", endl, serializer.GetString());

	return 0;
}*/

//#if INTRA_DISABLED
struct Test
{
	Array<int> intArray;
	int fixedIntArray[3];
	bool booleanVal;
	float flt;
	Array<String> stringArray;

	INTRA_ADD_REFLECTION(Test, intArray, fixedIntArray, booleanVal, flt, stringArray);
};

struct SuperTest
{
	Array<String> strArr;
	int foo;
	String str;
	Array<short> vals;
	double dbl;
	Test tests[3];
	ushort bar;
	Meta::Tuple<int, float, String> tuple;

	INTRA_ADD_REFLECTION(SuperTest, strArr, foo, str, vals, dbl, tests, bar, tuple);
};
static_assert(Meta::HasForEachField<const SuperTest&, Data::BinaryDeserializer&>::_, "ERROR!");



struct TestRef
{
	TestRef& operator=(const Test& rhs)
	{
		intArray = rhs.intArray;
		fixedIntArray[0] = rhs.fixedIntArray[0];
		fixedIntArray[1] = rhs.fixedIntArray[1];
		fixedIntArray[2] = rhs.fixedIntArray[2];
		booleanVal = rhs.booleanVal;
		flt = rhs.flt;
		stringArray.Clear();
		stringArray.AddLastRange(rhs.stringArray());
		return *this;
	}

	TestRef& operator=(const TestRef&) = default;

	ArrayRange<const int> intArray;
	int fixedIntArray[3];
	bool booleanVal;
	float flt;
	Array<StringView> stringArray;

	INTRA_ADD_REFLECTION(TestRef, intArray, fixedIntArray, booleanVal, flt, stringArray);
};

struct SuperTestRef
{
	SuperTestRef& operator=(const SuperTest& rhs)
	{
		strArr.Clear();
		strArr.AddLastRange(rhs.strArr());
		foo = rhs.foo;
		str = rhs.str;
		vals = rhs.vals;
		dbl = rhs.dbl;
		tests[0] = rhs.tests[0];
		tests[1] = rhs.tests[1];
		tests[2] = rhs.tests[2];
		bar = rhs.bar;
		Meta::Get<0>(tuple) = Meta::Get<0>(rhs.tuple);
		Meta::Get<1>(tuple) = Meta::Get<1>(rhs.tuple);
		Meta::Get<2>(tuple) = Meta::Get<2>(rhs.tuple);
		return *this;
	}

	SuperTestRef& operator=(const SuperTestRef&) = default;

	Array<StringView> strArr;
	int foo;
	StringView str;
	ArrayRange<const short> vals;
	double dbl;
	TestRef tests[3];
	ushort bar;
	Meta::Tuple<int, float, StringView> tuple;

	INTRA_ADD_REFLECTION(SuperTestRef, strArr, foo, str, vals, dbl, tests, bar, tuple);
};




static const StringView g_SuperTestTextWithNames = R"(
{
	.strArr = {"str1", "ergvwr", "brt"},
	.foo = 5,
	.str = "gammaker",
	.vals = {-4, 66, 432, -95},
	.dbl = 3.1415926535897932384626433832795,
	{
		{
			.fixedIntArray = {9, 4, 85},
			.stringArray = {"test 0 A", "test 0 B", "test 0 C"},
			.booleanVal = true,
			.intArray = {43, 54, 36, 76},
			.flt = 1.23456,
			.flt = 2.34567
		},
		{
			.intArray = {},
			.fixedIntArray = {3655456, 234, 3},
			.booleanVal = false,
			.flt = 2.718281828,
			.stringArray = {"test 1 A", "test 1 B"}
		},
		{
			.intArray = {1531, 1253, 16, -634, 236462363},
			.fixedIntArray = {9435, 435, 8355},
			.booleanVal = false,
			.flt = 123.65,
			.stringArray = {"test 2 A", "test 2 B", "test 2 C", "test 2 D"}
		}
	},
	.bar = 1025,
	.tuple = {123, 1.72123, "String in tuple!"}
})";

static const StringView g_SuperTestTextWithoutNames = R"({
	{"str1", "ergvwr", "brt"},
	5, "gammaker", {-4, 66, 432, -95},
	3.1415926535897932384626433832795,
	{
		{
			{43, 54, 36, 76},
			{9, 4, 85},
			true,
			1.23456,
			{"test 0 A", "test 0 B", "test 0 C"}
		},
		{
			{},
			{3655456, 234, 3},
			false,
			2.718281828,
			{"test 1 A", "test 1 B"}
		},
		{
			{1531, 1253, 16, 634, 236462363},
			{9435, 435, 8355},
			false,
			123.65,
			{"test 2 A", "test 2 B", "test 2 C", "test 2 D"}
		}
	}, 1025, {123, 1.72123, "String in tuple!"}
})";

SuperTest g_SuperTest;

SuperTestRef g_SuperTestRef;

double TestTextDeserialization(size_t times, const Data::DataLanguageParams& lang, const Data::TextSerializerParams& params)
{
	char buf[1000];
	Data::TextSerializer ser(lang, params, ArrayRange<char>(buf));
	ser(g_SuperTest);

	Data::TextDeserializer deserializer(lang, ser.GetString());
	SuperTest test = deserializer.Deserialize<SuperTest>();
	if(!deserializer.Log.Empty())
	{
		Console.PrintLine(endl, "В процессе десериализации произошли следующие ошибки: ");
		Console.PrintLine(deserializer.Log, endl);
	}

	Timer tim;
	for(size_t i=0; i<times; i++)
	{
		deserializer.ResetStream(ser.GetString());
		deserializer.Deserialize<SuperTest>();
	}
	return tim.GetTime();
}



double TestBinarySerialization(size_t times)
{
	char buf[1000];
	Data::BinarySerializer binser((ArrayRange<char>(buf)));
	Timer tim;
	for(size_t i=0; i<times; i++)
	{
		binser(g_SuperTest);
		binser.Output.Rest = ArrayRange<char>(buf);
	}
	return tim.GetTime();
}

double TestTextSerialization(Logger& logger, StringView desc, size_t times,
	const Data::DataLanguageParams& lang, const Data::TextSerializerParams& params)
{
	char buf[1000];
	Data::TextSerializer ser(lang, params, ArrayRange<char>(buf));
	Timer tim;
	for(size_t i=0; i<times; i++)
	{
		ser.ResetOutput(ArrayRange<char>(buf));
		ser(g_SuperTest);
	}
	double result = tim.GetTime();
	logger.BeginSpoiler("Результат сериализации в " + desc, "[Скрыть] Результат сериализации в " + desc);
	logger.PrintCode(ser.GetString());
	logger.EndSpoiler();
	return result;
}

double TestBinaryDeserialization(size_t times)
{
	char buf[1000];
	Data::BinarySerializer binser((ArrayRange<char>(buf)));
	binser(g_SuperTest);

	Data::BinaryDeserializer bindeser(binser.Output.GetRange());
	SuperTest newTest;
	Timer tim;
	for(size_t i=0; i<times; i++)
	{
		bindeser.Input.Rest = binser.Output.GetString();
		bindeser(newTest);
	}
	return tim.GetTime();
}

double TestBinaryRefDeserialization(size_t times)
{
	char buf[1000];
	Data::BinarySerializer binser((ArrayRange<char>(buf)));
	binser(g_SuperTestRef);

	Data::BinaryDeserializer bindeser(binser.Output.GetRange());
	SuperTestRef newTest;
	Timer tim;
	for(size_t i=0; i<times; i++)
	{
		bindeser.Input.Rest = binser.Output.GetString();
		bindeser(newTest);
	}
	return tim.GetTime();
}



void RunSerializationPerfTests(IO::Logger& logger)
{
	logger.BeginSpoiler("Строка для десериализации структуры", "[Скрыть] Строка для десериализации структуры");
	logger.PrintCode(g_SuperTestTextWithNames);
	logger.EndSpoiler();

	Data::TextDeserializer deserializer(Data::DataLanguageParams::CStructInitializer, g_SuperTestTextWithNames);
	deserializer(g_SuperTest);
	g_SuperTestRef = g_SuperTest;
	if(!deserializer.Log.Empty())
	{
		logger.PushFont({128,0,0}, 3.5f);
		logger.PrintLine(endl, "В процессе десериализации произошли следующие ошибки: ");
		logger.PopFont();
		logger.BeginSpoiler("Лог сериализатора", "[Скрыть] Лог сериализатора");
		logger.PrintCode(deserializer.Log);
		logger.EndSpoiler();
	}

	if(TestGroup gr{logger, "Бинарная сериализация"})
	{
		PrintPerformanceResults(logger, "Сериализация структуры 1000000 раз",
			{"binary"}, null,
			{
				TestBinarySerialization(1000000)
			});
	}

	if(TestGroup gr{logger, "Бинарная десериализация"})
	{
		PrintPerformanceResults(logger, "Десериализация структуры с контейнерами 1000000 раз",
			{"binary"}, null,
			{
				TestBinaryDeserialization(1000000)
			});

		PrintPerformanceResults(logger, "Десериализация структуры с диапазонами 1000000 раз",
			{"binary"}, null,
			{
				TestBinaryRefDeserialization(1000000)
			});
	}

	if(TestGroup gr{logger, "Текстовая сериализация"})
	{
		PrintPerformanceResults(logger, "Сериализация структуры 1000000 раз",
			{"(1) C struct", "(2) JSON", "(3) JSON compact", "(4) XML subset", "(5) JSON-like custom"}, null,
			{
				TestTextSerialization(logger, "(1) C struct", 1000000,
					Data::DataLanguageParams::CStructInitializer, Data::TextSerializerParams::Verbose),
				TestTextSerialization(logger, "(2) JSON", 1000000,
					Data::DataLanguageParams::Json, Data::TextSerializerParams::Verbose),
				TestTextSerialization(logger, "(3) JSON compact", 1000000,
					Data::DataLanguageParams::Json, Data::TextSerializerParams::Compact),
				TestTextSerialization(logger, "(4) XML subset", 1000000,
					Data::DataLanguageParams::Xml, Data::TextSerializerParams::Verbose),
				TestTextSerialization(logger, "(5) JSON-like custom", 1000000,
					Data::DataLanguageParams::JsonLikeNoQuotes, Data::TextSerializerParams::Verbose)
			});
	}

	if(TestGroup gr{logger, "Текстовая десериализация"})
	{
		PrintPerformanceResults(logger, "Десериализация структуры 100000 раз",
			{"(1) C struct", "(2) JSON", "(3) JSON compact", "(4) XML subset", "(5) JSON-like custom"}, null,
			{
				TestTextDeserialization(100000, Data::DataLanguageParams::CStructInitializer, Data::TextSerializerParams::Verbose),
				TestTextDeserialization(100000, Data::DataLanguageParams::Json, Data::TextSerializerParams::Verbose),
				TestTextDeserialization(100000, Data::DataLanguageParams::Json, Data::TextSerializerParams::Compact),
				TestTextDeserialization(100000, Data::DataLanguageParams::Xml, Data::TextSerializerParams::Verbose),
				TestTextDeserialization(100000, Data::DataLanguageParams::JsonLikeNoQuotes, Data::TextSerializerParams::Verbose)
			});
	}

}

