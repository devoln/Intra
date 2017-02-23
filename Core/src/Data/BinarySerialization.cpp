#include "Data/Serialization/BinarySerializer.h"
#include "Data/Reflection.h"
#include "Test/Unittest.h"
#include "Platform/CppWarnings.h"

//TODO: вынести этот файл в отдельный проект для Unit-тестов

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifdef INTRA_RUN_UNITTESTS

#include "Container/Sequential/Array.h"
#include "Container/Sequential/String.h"

namespace Intra { namespace Data {

INTRA_UNITTEST("Binary serialization and deserialization")
{
	byte data[1000];
	Data::BinarySerializer serializer(data);
	int originalA = 12345;
	float originalB = 3.1415926f;
	StringView originalC = "C";
	String originalD = "D";
	int originalE[] = {3234, 23, 423, 423, 22};
	ArrayRange<const int> originalF = originalE;
	Array<int> originalG = originalF;
	Meta::Tuple<int, double> originalH = {-8543211, 2.718281828};

	serializer << originalA << originalB << originalC << originalD << originalE << originalF;


	Data::BinaryDeserializer deserializer(serializer.Output.GetWrittenData());
	int copyA;
	deserializer >> copyA;
	INTRA_TEST_ASSERT_EQUALS(originalA, copyA);

	float copyB;
	deserializer >> copyB;
	INTRA_TEST_ASSERT_EQUALS(originalB, copyB);

	StringView copyC;
	deserializer >> copyC;
	INTRA_TEST_ASSERT_EQUALS(originalC, copyC);
	
	String copyD;
	deserializer >> copyD;
	INTRA_TEST_ASSERT_EQUALS(originalD, copyD);
	
	int copyE[5];
	deserializer >> copyE;
	INTRA_TEST_ASSERT(Algo::Equals(originalE, copyE));
	
	ArrayRange<const int> copyF;
	deserializer >> copyF;
	Algo::MaxLengthOfToString(copyF);
	INTRA_TEST_ASSERT_EQUALS(originalF, copyF);
	
	Array<int> copyG;
	deserializer >> copyG;
	INTRA_TEST_ASSERT_EQUALS(originalG, copyG);

	Meta::Tuple<int, double> copyH;
	deserializer >> copyH;
	INTRA_TEST_ASSERT_EQUALS(originalH, copyH);
};

}}

#endif

INTRA_WARNING_POP
