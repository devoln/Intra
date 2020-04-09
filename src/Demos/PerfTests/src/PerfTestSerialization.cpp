#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include "PerfTestSerialization.h"

#include "Extra/Unstable/Data/Serialization.hh"
#include "Intra/Reflection.h"
#include "Extra/System/Stopwatch.h"
#include "Intra/Range/Span.h"
#include "Test/PerfSummary.h"
#include "Test/TestGroup.h"
#include "Extra/IO/Std.h"

using namespace Intra;

INTRA_DISABLE_REDUNDANT_WARNINGS

//#if INTRA_DISABLED
struct Test
{
	Array<int> intArray;
	int fixedIntArray[3];
	bool booleanVal;
	float flt;
	Array<String> stringArray;

	INTRA_ADD_FIELD_REFLECTION(Test, intArray, fixedIntArray, booleanVal, flt, stringArray)
};

struct SuperTest
{
	Array<String> strArr;
	int foo;
	String str;
	Array<short> vals;
	double dbl;
	Test tests[3];
	uint16 bar;
	Tuple<int, float, String> tuple;

	INTRA_ADD_FIELD_REFLECTION(SuperTest, strArr, foo, str, vals, dbl, tests, bar, tuple)
};



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
		stringArray.AddLastRange(rhs.stringArray);
		return *this;
	}

	TestRef& operator=(const TestRef&) = default;

	CSpan<int> intArray;
	int fixedIntArray[3];
	bool booleanVal;
	float flt;
	Array<StringView> stringArray;

	INTRA_ADD_FIELD_REFLECTION(TestRef, intArray, fixedIntArray, booleanVal, flt, stringArray)
};

struct SuperTestRef
{
	SuperTestRef& operator=(const SuperTest& rhs)
	{
		strArr.Clear();
		strArr.AddLastRange(rhs.strArr);
		foo = rhs.foo;
		str = rhs.str;
		vals = rhs.vals;
		dbl = rhs.dbl;
		tests[0] = rhs.tests[0];
		tests[1] = rhs.tests[1];
		tests[2] = rhs.tests[2];
		bar = rhs.bar;
		tuple.Field<0>() = rhs.tuple.Field<0>();
		tuple.Field<1>() = rhs.tuple.Field<1>();
		tuple.Field<2>() = rhs.tuple.Field<2>();
		return *this;
	}

	SuperTestRef& operator=(const SuperTestRef&) = default;

	Array<StringView> strArr;
	int foo;
	StringView str;
	CSpan<short> vals;
	double dbl;
	TestRef tests[3];
	uint16 bar;
	Tuple<int, float, StringView> tuple;

	INTRA_ADD_FIELD_REFLECTION(SuperTestRef, strArr, foo, str, vals, dbl, tests, bar, tuple)
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

double TestTextDeserialization(size_t times,
	const LanguageParams& lang,
	const TextSerializerParams& params)
{
	char buf[1000];
	TextSerializer ser(lang, params, Span<char>(buf));
	ser << g_SuperTest;

	auto deserializer = TextDeserializer(lang, StringView(ser.Output.WrittenRange()));
	SuperTest test = deserializer.Deserialize<SuperTest>();
	if(!deserializer.Log.Empty())
	{
		Std.LineBreak();
		Std.PrintLine("The following errors occured during deserialization: ");
		Std.PrintCode(deserializer.Log);
		Std.LineBreak();
	}

	Stopwatch tim;
	for(size_t i=0; i<times; i++)
	{
		deserializer.ResetStream(StringView(ser.Output.WrittenRange()));
		deserializer.Deserialize<SuperTest>();
	}
	return tim.ElapsedSeconds();
}



double TestBinarySerialization(size_t times)
{
	byte buf[1000];
	BinarySerializer binser(buf);
	Stopwatch tim;
	for(size_t i=0; i<times; i++)
	{
		binser << g_SuperTest;
		binser.Output.Reset();
	}
	return tim.ElapsedSeconds();
}

double TestTextSerialization(FormattedWriter& logger, StringView desc, size_t times,
	const LanguageParams& lang, const TextSerializerParams& params)
{
	char buf[1000];
	TextSerializer ser(lang, params, Span<char>(buf));
	Stopwatch tim;
	for(size_t i=0; i<times; i++)
	{
		ser.ResetOutput(Span<char>(buf));
		ser << g_SuperTest;
	}
	double result = tim.ElapsedSeconds();
	logger.BeginSpoiler("Serialization result to " + desc);
	logger.PrintCode(ser.Output.WrittenRange());
	logger.EndSpoiler();
	return result;
}

double TestBinaryDeserialization(size_t times)
{
	byte buf[1000];
	BinarySerializer binser(buf);
	binser << g_SuperTest;

	BinaryDeserializer bindeser(binser.Output.WrittenRange());
	SuperTest newTest;
	Stopwatch tim;
	for(size_t i=0; i<times; i++)
	{
		bindeser.Input = binser.Output.WrittenRange();
		bindeser >> newTest;
	}
	return tim.ElapsedSeconds();
}

double TestBinaryRefDeserialization(size_t times)
{
	byte buf[1000];
	BinarySerializer binser(buf);
	binser << g_SuperTestRef;

	BinaryDeserializer bindeser(binser.Output.WrittenRange());
	SuperTestRef newTest;
	Stopwatch tim;
	for(size_t i=0; i<times; i++)
	{
		bindeser.Input = binser.Output.WrittenRange();
		bindeser >> newTest;
	}
	return tim.ElapsedSeconds();
}



void RunSerializationPerfTests(FormattedWriter& output)
{
	output.BeginSpoiler("String with structure description to deserialize: ");

	output.PrintCode(g_SuperTestTextWithNames);
	output.EndSpoiler();

	auto deserializer = TextDeserializer(LanguageParams::CStructInitializer, g_SuperTestTextWithNames);
	deserializer >> g_SuperTest;
	g_SuperTestRef = g_SuperTest;
	if(!deserializer.Log.Empty())
	{
		output.PushFont({128,0,0}, 3.5f);
		output.LineBreak();
		output.PrintLine("The folowing errors occured during deserialization: ");
		output.PopFont();
		output.BeginSpoiler("Deserializer log");
		output.PrintCode(deserializer.Log);
		output.EndSpoiler();
	}

	if(TestGroup gr{"Binary serialization"})
	{
		PrintPerformanceResults(output, "Serializing struct 1000000 times",
			{"binary"}, null,
			{
				TestBinarySerialization(1000000)
			});
	}

	if(TestGroup gr{"Binary deserialization"})
	{
		PrintPerformanceResults(output, "Deserializing struct with containers 1000000 times",
			{"binary"}, null,
			{
				TestBinaryDeserialization(1000000)
			});

		PrintPerformanceResults(output, "Deserializing struct with ranges 1000000 times",
			{"binary"}, null,
			{
				TestBinaryRefDeserialization(1000000)
			});
	}

	if(TestGroup gr{"Text serialization"})
	{
		PrintPerformanceResults(output, "Serializing struct 1000000 times",
			{"(1) C struct", "(2) JSON", "(3) JSON compact", "(4) XML subset", "(5) JSON-like custom"}, null,
			{
				TestTextSerialization(output, "(1) C struct", 1000000,
					LanguageParams::CStructInitializer, TextSerializerParams::Verbose),
				TestTextSerialization(output, "(2) JSON", 1000000,
					LanguageParams::Json, TextSerializerParams::Verbose),
				TestTextSerialization(output, "(3) JSON compact", 1000000,
					LanguageParams::Json, TextSerializerParams::Compact),
				TestTextSerialization(output, "(4) XML subset", 1000000,
					LanguageParams::Xml, TextSerializerParams::Verbose),
				TestTextSerialization(output, "(5) JSON-like custom", 1000000,
					LanguageParams::JsonLikeNoQuotes, TextSerializerParams::Verbose)
			});
	}

	if(TestGroup gr{"Text deserialization"})
	{
		TestTextDeserialization(1, LanguageParams::Xml, TextSerializerParams::Verbose);
		PrintPerformanceResults(output, "Deserializing struct 100000 times",
			{"(1) C struct", "(2) JSON", "(3) JSON compact", "(4) XML subset", "(5) JSON-like custom"}, null,
			{
				TestTextDeserialization(100000, LanguageParams::CStructInitializer, TextSerializerParams::Verbose),
				TestTextDeserialization(100000, LanguageParams::Json, TextSerializerParams::Verbose),
				TestTextDeserialization(100000, LanguageParams::Json, TextSerializerParams::Compact),
				TestTextDeserialization(100000, LanguageParams::Xml, TextSerializerParams::Verbose),
				TestTextDeserialization(100000, LanguageParams::JsonLikeNoQuotes, TextSerializerParams::Verbose)
			});
	}

}

