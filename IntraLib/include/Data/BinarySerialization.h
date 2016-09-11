#pragma once

#include "Platform/Endianess.h"
#include "Meta/Preprocessor.h"
#include "Meta/Tuple.h"
#include "Meta/Type.h"
#include "IO/StaticStream.h"
#include "Data/Reflection.h"
#include "Containers/String.h"

namespace Intra { namespace Data {

template<typename O> class GenericBinarySerializer
{
public:
	GenericBinarySerializer(const O& output): Output(output) {}

	//! Сериализовать массив простым копированием байт.
	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_
	> SerializeArray(ArrayRange<T> v)
	{
		Output.template WriteRaw<uintLE>((uint)v.Length());
		Output.WriteRaw(v.Begin, v.Length()*sizeof(T));
	}

	//! Сериализовать массив поэлементно.
	template<typename T> Meta::EnableIf<
		!Meta::IsTriviallySerializable<T>::_
	> SerializeArray(ArrayRange<T> v) {SerializeRange(v);}

	//! Сериализовать ForwardRange поэлементно.
	template<typename ForwardRange> void SerializeRange(const ForwardRange& v)
	{
		Output.template WriteRaw<uintLE>((uint)v.Count());
		ForwardRange range = v;
		while(!range.Empty())
		{
			operator()(range.First());
			range.PopFirst();
		}
	}

	//! Сериализовать любое значение.
	template<typename T> forceinline GenericBinarySerializer& operator()(const T& value)
	{
		SerializeBinary(*this, value);
		return *this;
	}

	//! Получить размер в байтах, который займёт элемент value при сериализации без учёта выравнивания.
	template<typename T> static size_t SerializedSizeOf(const T& value)
	{
		GenericBinarySerializer<IO::DummyOutput> dummy({});
		dummy(value);
		return dummy.Output.BytesWritten();
	}

	O Output;
};
typedef GenericBinarySerializer<IO::MemoryOutput> BinarySerializer;
typedef GenericBinarySerializer<IO::DummyOutput> DummyBinarySerializer;


class BinaryDeserializer
{
public:
	BinaryDeserializer(ArrayRange<const char> input): Input(input) {}

	//! Поэлементно десериализовать массив длиной count в OutputRange dst.
	template<typename OutputRange> OutputRange DeserializeToOutputRange(const OutputRange& dst, size_t count)
	{
		typedef typename OutputRange::value_type T;
		OutputRange dstCopy = dst;
		for(size_t i=0; i<count; i++)
			dstCopy.Put(Deserialize<T>());
		return dstCopy;
	}


	//! Десериализовать любое значение по ссылке на него.
	template<typename T> forceinline BinaryDeserializer& operator()(T& value)
	{
		DeserializeBinary(*this, value);
		return *this;
	}

	//! Десериализовать любое значение и вернуть его из метода.
	template<typename T> forceinline T Deserialize()
	{
		T result;
		DeserializeBinary(*this, result);
		return result;
	}

	IO::MemoryInput Input;
};


struct StructReflection;

//! Сериализовать runtime структуру
void SerializeStructBinary(BinarySerializer& serializer, const void* src, const StructReflection& reflection);
void SerializeStructBinary(DummyBinarySerializer& serializer, const void* src, const StructReflection& reflection);

//! Десериализовать runtime структуру
void DeserializeStructBinary(BinaryDeserializer& deserializer, const StructReflection& reflection, void*& dst);



//! Сериализация для тривиально сериализуемых типов.
template<typename T, typename O> forceinline Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_
> SerializeBinary(GenericBinarySerializer<O>& serializer, const T& v)
{
	serializer.Output.WriteRaw(&v, sizeof(T));
}

//! Десериализация для тривиально сериализуемых типов.
template<typename T> forceinline Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_
> DeserializeBinary(BinaryDeserializer& deserializer, T& dst)
{
	deserializer.Input.ReadRaw(&dst, sizeof(T)); 
}


//! Сериализация диапазонов, представленных непрерывно в памяти.
template<typename O, typename AnyArrayRange> forceinline Meta::EnableIf<
	Range::IsArrayRange<AnyArrayRange>::_ || Range::HasAsRange<AnyArrayRange>::_
> SerializeBinary(GenericBinarySerializer<O>& serializer, const AnyArrayRange& v)
{
	auto range = Range::AsRange(v);
	serializer.SerializeArray(ArrayRange<const typename decltype(range)::value_type>(range.Data(), range.Length()));
}





//! Десериализация GenericStringView. Результат ссылается на данные в сериализованной области.
template<typename Char> void DeserializeBinary(BinaryDeserializer& deserializer, GenericStringView<Char>& dst)
{
	ArrayRange<const Char> result;
	deserializer(result);
	dst = {result.Begin, result.End};
}

//! Десериализация класса строки.
template<typename Char, class Allocator> void DeserializeBinary(BinaryDeserializer& deserializer, GenericString<Char, Allocator>& dst)
{
	uint len = deserializer.Deserialize<uintLE>();
	dst.SetLengthUninitialized(len);
	deserializer.Input.ReadRaw(dst.Data(), len*sizeof(Char));
}



//! Десериализовать массив в ArrayRange.
//! Это можно делать только для тривиально сериализуемых типов, так как полученный dst ссылается на данные в сериализованной области.
template<typename T> Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_
> DeserializeBinary(BinaryDeserializer& deserializer, ArrayRange<const T>& dst)
{
	uint count = deserializer.Deserialize<uintLE>();
	dst = ArrayRange<const T>(reinterpret_cast<const T*>(deserializer.Input.Rest.Data()), count);
	deserializer.Input.Rest.PopFirstExactly(count*sizeof(T));
}

//! Десериализовать массив
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallySerializable<T>::_
> DeserializeBinary(BinaryDeserializer& deserializer, Array<T>& dst)
{
	uint count = deserializer.Deserialize<uintLE>();
	dst.Reserve(count);
	deserializer.DeserializeToOutputRange(dst.Insert($), count);
}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_
> DeserializeBinary(BinaryDeserializer& deserializer, Array<T>& dst)
{
	uint count = deserializer.Deserialize<uintLE>();
	dst.SetCountUninitialized(count);
	deserializer.Input.ReadRaw(dst.Data(), count*sizeof(T));
}



//! Сериализовать нетривиально сериализуемую структуру или класс со статической рефлексией.
template<typename T, typename O> forceinline Meta::EnableIf<
	(HasReflection<T>::_ || Meta::IsTuple<T>::_) && !Meta::IsTriviallySerializable<T>::_
> SerializeBinary(GenericBinarySerializer<O>& serializer, const T& src)
{
	src.ForEachField(serializer);
}


//! Десериализовать нетривиально сериализуемую структуру или класс со статической рефлексией.
template<typename T> Meta::EnableIf<
	(HasReflection<T>::_ || Meta::IsTuple<T>::_) && !Meta::IsTriviallySerializable<T>::_
> DeserializeBinary(BinaryDeserializer& deserializer, T& dst)
{
	dst.ForEachField(deserializer);
}


//! Сериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
template<typename T, size_t N, typename O> forceinline Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_
> SerializeBinary(GenericBinarySerializer<O>& serializer, const T(&src)[N])
{
	serializer.Output.WriteRaw(src, N*sizeof(T));
}

//! Поэлементно сериализовать массив фиксированной длины нетривиально сериализуемого типа.
template<typename T, size_t N, typename O> forceinline Meta::EnableIf<
	!Meta::IsTriviallySerializable<T>::_
> SerializeBinary(GenericBinarySerializer<O>& serializer, const T(&src)[N])
{
	for(size_t i=0; i<N; i++) serializer(src[i]);
}

//! Десериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
template<typename T, size_t N> Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_
> DeserializeBinary(BinaryDeserializer& deserializer, T(&dst)[N])
{
	deserializer.Input.ReadRaw(dst, N*sizeof(T));
}

//! Поэлементно десериализовать массив фиксированной длины нетривально десериализуемого типа.
template<typename T, size_t N> Meta::EnableIf<
	!Meta::IsTriviallySerializable<T>::_
> DeserializeBinary(BinaryDeserializer& deserializer, T(&dst)[N])
{
	for(size_t i=0; i<N; i++) deserializer(dst[i]);
}

}}

