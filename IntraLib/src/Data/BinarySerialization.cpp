#include "Data/BinarySerialization.h"
#include "Data/Reflection.h"
#include "Test/UnitTest.h"

namespace Intra { namespace Data {

#if INTRA_DISABLED
template<typename O> static void serializeStructBinary(
	GenericBinarySerializer<O>& serializer, const void* src, const StructReflection& reflection)
{
	for(size_t i=0; i<reflection.Fields.Length(); i++)
	{
		auto& f = reflection.Fields[i];
		auto fieldSrc = (const byte*)src+f.Offset;

		const bool thisTypeHasReflection = (f.Type==ValueType::StructureInstance ||
			f.Type==ValueType::End) && f.SubstructReflection!=null;
		if(thisTypeHasReflection)
		{
			SerializeStructBinary(serializer, fieldSrc, *f.SubstructReflection);
			continue;
		}

		//String или StringView
		if(f.Type==ValueType::String || f.Type==ValueType::StringView)
		{
			SerializeBinary(serializer, *(const StringView*)fieldSrc);
			continue;
		}

		//Это какой-то сторонний тип без рефлексии, но поле имеет заданный сериализатор
		if(f.Type==ValueType::End)
		{
			INTRA_ASSERT(reflection.FieldSerializers[i]!=null);
			reflection.FieldSerializers[i]->SerializeBinary(serializer, fieldSrc);
			continue;
		}

		//Это POD структура без рефлексии. Тогда сохраняются все поля как есть, даже (!) указатели
		if(f.Type==ValueType::StructureInstance || f.Type.IsPod())
		{
			serializer.Output.WriteRaw(fieldSrc, f.Size);
			continue;
		}

		INTRA_ASSERT(!"Обработка поля данного типа не реализована!");
	}
}
#endif

#if INTRA_DISABLED
void SerializeStructBinary(BinarySerializer& serializer, const void* src, const StructReflection& reflection)
{
	serializeStructBinary(serializer, src, reflection);
}

void SerializeStructBinary(DummyBinarySerializer& serializer, const void* src, const StructReflection& reflection)
{
	serializeStructBinary(serializer, src, reflection);
}

void DeserializeStructBinary(BinaryDeserializer& deserializer, const StructReflection& reflection, void*& dst)
{
	for(size_t i=0; i<reflection.Fields.Length(); i++)
	{
		auto& f = reflection.Fields[i];
		auto dstField = (byte*)dst+f.Offset;

		//Если это структура или класс с рефлексией
		if((f.Type==ValueType::StructureInstance || f.Type==ValueType::End) && f.SubstructReflection!=null)
		{
			void* fieldPtr = dstField;
			DeserializeStructBinary(deserializer, *f.SubstructReflection, fieldPtr);
			continue;
		}

		if(f.Type==ValueType::String)
		{
			deserializer.Deserialize(*reinterpret_cast<String*>(dstField));
			continue;
		}
		if(f.Type==ValueType::StringView)
		{
			deserializer.Deserialize(*reinterpret_cast<StringView*>(dstField));
			continue;
		}

		//Это какой-то сторонний тип без рефлексии, но поле имеет заданный сериализатор
		if(f.Type==ValueType::End)
		{
			INTRA_ASSERT(reflection.FieldSerializers[i]!=null);
			reflection.FieldSerializers[i]->DeserializeBinary(deserializer, dstField);
			continue;
		}

		if(f.Type==ValueType::StructureInstance || f.Type.IsPod())
		{
			deserializer.Input.ReadRaw(dstField, f.Size);
			continue;
		}

		INTRA_ASSERT(!"Обработка поля данного типа не реализована!");
	}
}
#endif

}}

#ifdef INTRA_RUN_UNITTESTS

#include "Containers/Array.h"
#include "Containers/String.h"

namespace Intra {

INTRA_UNITTEST("Binary serialization and deserialization")
{
	byte data[1000];
	Data::BinarySerializer serializer((ArrayRange<byte>(data)));
	int originalA = 12345;
	float originalB = 3.1415926f;
	StringView originalC = "C";
	String originalD = "D";
	int originalE[] = {3234, 23, 423, 423, 22};
	ArrayRange<const int> originalF = originalE;
	Array<int> originalG = originalF;
	Meta::Tuple<int, double> originalH = {-8543211, 2.718281828};

	serializer(originalA)(originalB)(originalC)(originalD)(originalE)(originalF);


	Data::BinaryDeserializer deserializer(serializer.Output.GetRange());
	int copyA;
	deserializer(copyA);
	INTRA_TEST_ASSERT_EQUALS(originalA, copyA);

	float copyB;
	deserializer(copyB);
	INTRA_TEST_ASSERT_EQUALS(originalB, copyB);

	StringView copyC;
	deserializer(copyC);
	INTRA_TEST_ASSERT_EQUALS(originalC, copyC);
	
	String copyD;
	deserializer(copyD);
	INTRA_TEST_ASSERT_EQUALS(originalD, copyD);
	
	int copyE[5];
	deserializer(copyE);
	INTRA_TEST_ASSERT(Range::Equals(AsRange(originalE), AsRange(copyE)));
	
	ArrayRange<const int> copyF;
	deserializer(copyF);
	INTRA_TEST_ASSERT_EQUALS(originalF, copyF);
	
	Array<int> copyG;
	deserializer(copyG);
	INTRA_TEST_ASSERT_EQUALS(originalG, copyG);

	Meta::Tuple<int, double> copyH;
	deserializer(copyH);
	INTRA_TEST_ASSERT_EQUALS(originalH, copyH);
};

}

#endif
