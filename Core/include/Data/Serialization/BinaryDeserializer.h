#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/Endianess.h"
#include "Meta/Preprocessor.h"
#include "Meta/Type.h"
#include "Meta/EachField.h"
#include "IO/StaticStream.h"
#include "Data/Reflection.h"
#include "Containers/String.h"
#include "Range/Operations.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class BinaryDeserializer
{
public:
	BinaryDeserializer(ArrayRange<const char> input): Input(input) {}

	//! Поэлементно десериализовать массив длиной count в OutputRange dst.
	template<typename OR> Meta::EnableIf<
		Range::IsOutputRange<OR>::_
	> DeserializeToOutputRange(OR&& dst, size_t count)
	{
		typedef Range::ValueTypeOf<OR> T;
		while(count --> 0)
			dst.Put(Deserialize<T>());
	}


	//! Десериализовать любое значение по ссылке на него.
	//! Этот оператор нужен, чтобы использовать десериализатор как функтор и передавать в алгоритмы.
	template<typename T> forceinline Meta::EnableIf<
		!Meta::IsConst<T>::_,
	BinaryDeserializer&> operator()(T&& value)
	{return *this >> Meta::Forward<T>(value);}

	//! Десериализовать любое значение и вернуть его из метода.
	template<typename T> forceinline T Deserialize()
	{
		T result;
		*this >> result;
		return result;
	}

	IO::MemoryInput Input;
};


//! Десериализовать runtime структуру
//void DeserializeStructBinary(BinaryDeserializer& deserializer, const StructReflection& reflection, void*& dst);


//! Десериализация для тривиально сериализуемых типов.
template<typename T> forceinline Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_ && !Range::IsInputRange<T>::_,
BinaryDeserializer&> operator>>(BinaryDeserializer& deserializer, T& dst)
{
	deserializer.Input.ReadRaw(&dst, sizeof(T));
	return deserializer;
}

//! Десериализация GenericStringView. Результат ссылается на данные в сериализованной области.
template<typename Char> BinaryDeserializer& operator>>(
	BinaryDeserializer& deserializer, GenericStringView<Char>& dst)
{
	ArrayRange<const Char> result;
	deserializer >> result;
	dst = {result.Begin, result.End};
	return deserializer;
}

//! Десериализация класса строки.
template<typename Char, class Allocator> BinaryDeserializer& operator>>(
	BinaryDeserializer& deserializer, GenericString<Char, Allocator>& dst)
{
	uint len = deserializer.Deserialize<uintLE>();
	dst.SetLengthUninitialized(len);
	deserializer.Input.ReadRaw(dst.Data(), len*sizeof(Char));
	return deserializer;
}



//! Десериализовать массив в ArrayRange.
//! Это можно делать только для тривиально сериализуемых типов,
//! так как полученный dst ссылается на данные в сериализованной области.
template<typename T> Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_,
BinaryDeserializer&> operator>>(BinaryDeserializer& deserializer, ArrayRange<const T>& dst)
{
	uint count = deserializer.Deserialize<uintLE>();
	dst = ArrayRange<const T>(reinterpret_cast<const T*>(deserializer.Input.Rest.Data()), count);
	deserializer.Input.Rest.PopFirstExactly(count*sizeof(T));
	return deserializer;
}

//! Десериализовать массив
template<typename C> Meta::EnableIf<
	Container::IsLastAppendable<C>::_ &&
	Container::IsClearable<C>::_ &&
	!(Container::IsResizable<C>::_ &&
		Container::HasData<C>::_ &&
		Meta::IsTriviallySerializable<Range::ValueTypeOfAs<C>>::_),
BinaryDeserializer&> operator>>(BinaryDeserializer& deserializer, C& dst)
{
	uint count = deserializer.Deserialize<uintLE>();
	Container::SetCount0(dst);
	Container::Reserve(dst, count);
	deserializer.DeserializeToOutputRange(Range::LastAppender(dst), count);
	return deserializer;
}

template<typename C> Meta::EnableIf<
	Container::IsResizable<C>::_ &&
	Container::HasData<C>::_ &&
	Meta::IsTriviallySerializable<Range::ValueTypeOfAs<C>>::_,
BinaryDeserializer&> operator>>(BinaryDeserializer& deserializer, C& dst)
{
	uint count = deserializer.Deserialize<uintLE>();
	Container::SetCount(dst, count);
	deserializer.Input.ReadRaw(dst.Data(), count*sizeof(Range::ValueTypeOfAs<C>));
	return deserializer;
}

//! Десериализовать нетривиально сериализуемую структуру или класс со статической рефлексией.
template<typename T> Meta::EnableIf<
	Meta::HasForEachField<T&, BinaryDeserializer&>::_ &&
	!Meta::IsTriviallySerializable<T>::_,
BinaryDeserializer&> operator>>(BinaryDeserializer& deserializer, T& dst)
{
	Meta::ForEachField(dst, deserializer);
	return deserializer;
}


//! Десериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
template<typename T, size_t N> Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_,
BinaryDeserializer&> operator>>(BinaryDeserializer& deserializer, T(&dst)[N])
{
	deserializer.Input.ReadRaw(dst, N*sizeof(T));
	return deserializer;
}

//! Поэлементно десериализовать массив фиксированной длины нетривально десериализуемого типа.
template<typename T, size_t N> Meta::EnableIf<
	!Meta::IsTriviallySerializable<T>::_,
BinaryDeserializer&> operator>>(BinaryDeserializer& deserializer, T(&dst)[N])
{
	Algo::ForEach(dst, deserializer);
	return deserializer;
}


INTRA_WARNING_POP

}}
