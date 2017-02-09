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
#include "Algo/ForEach.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename O> class GenericBinarySerializer
{
public:
	GenericBinarySerializer(const O& output): Output(output) {}

	//! Сериализовать массив простым копированием байт.
	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_
	> SerializeArray(ArrayRange<T> v)
	{
		Output.template WriteRaw<uintLE>(uint(v.Length()));
		Output.WriteRaw(v.Begin, v.Length()*sizeof(T));
	}

	//! Сериализовать массив поэлементно.
	template<typename T> Meta::EnableIf<
		!Meta::IsTriviallySerializable<T>::_
	> SerializeArray(ArrayRange<T> v) {SerializeRange(v);}

	//! Сериализовать диапазон поэлементно.
	template<typename R> void SerializeRange(R&& v)
	{
		Output.template WriteRaw<uintLE>(uint(Range::Count(v)));
		Algo::ForEach(Meta::Forward<R>(v), *this);
	}

	//! Сериализовать любое значение. Благодаря этому оператору класс может вести себя как функтор.
	template<typename T> forceinline GenericBinarySerializer& operator()(T&& value)
	{return *this << Meta::Forward<T>(value);}

	//! Сериализовать любое значение.
	//! Рекомендуется использовать этот метод вместо операторов () или << и явно указывать тип.
	template<typename T> forceinline GenericBinarySerializer& Serialize(T&& value)
	{return *this << Meta::Forward<T>(value);}

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


//struct StructReflection;

//! Сериализовать runtime структуру
//void SerializeStructBinary(BinarySerializer& serializer, const void* src, const StructReflection& reflection);
//void SerializeStructBinary(DummyBinarySerializer& serializer, const void* src, const StructReflection& reflection);




//! Сериализация для тривиально сериализуемых типов.
template<typename T, typename O> forceinline Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_ && !Range::IsInputRange<T>::_,
GenericBinarySerializer<O>&> operator<<(GenericBinarySerializer<O>& serializer, const T& v)
{
	serializer.Output.WriteRaw(&v, sizeof(T));
	return serializer;
}


//! Сериализация диапазонов, представленных непрерывно в памяти.
template<typename O, typename R> forceinline Meta::EnableIf<
	Range::IsAsArrayRange<R>::_,
GenericBinarySerializer<O>&> operator<<(GenericBinarySerializer<O>& serializer, R&& v)
{
	auto range = Range::Forward<R>(v);
	typedef Range::ValueTypeOfAs<R> T;
	serializer.SerializeArray(ArrayRange<const T>(range.Data(), range.Length()));
	return serializer;
}



//! Сериализовать нетривиально сериализуемую структуру или класс со статической рефлексией.
template<typename T, typename O> forceinline Meta::EnableIf<
	!Range::IsAsInputRange<T>::_ &&
	Meta::HasForEachField<const T&, GenericBinarySerializer<O>&>::_ &&
	!Meta::IsTriviallySerializable<T>::_,
GenericBinarySerializer<O>&> operator<<(GenericBinarySerializer<O>& serializer, const T& src)
{
	Meta::ForEachField(src, serializer);
	return serializer;
}

//! Сериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
template<typename T, size_t N, typename O> forceinline Meta::EnableIf<
	Meta::IsTriviallySerializable<T>::_,
GenericBinarySerializer<O>&> operator<<(GenericBinarySerializer<O>& serializer, const T(&src)[N])
{
	serializer.Output.WriteRaw(src, N*sizeof(T));
	return serializer;
}

//! Поэлементно сериализовать массив фиксированной длины нетривиально сериализуемого типа.
template<typename T, size_t N, typename O> forceinline Meta::EnableIf<
	!Meta::IsTriviallySerializable<T>::_,
GenericBinarySerializer<O>&> operator<<(GenericBinarySerializer<O>& serializer, const T(&src)[N])
{
	Algo::ForEach(src, serializer);
	return serializer;
}

INTRA_WARNING_POP

}}
