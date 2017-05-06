#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Endianess.h"

#include "Data/Reflection.h"

#include "Preprocessor/Preprocessor.h"

#include "Meta/Type.h"
#include "Meta/EachField.h"

#include "Utils/Span.h"

#include "Concepts/Container.h"
#include "Concepts/Range.h"

#include "Range/Operations.h"
#include "Range/ForEach.h"
#include "Range/Output/OutputArrayRange.h"
#include "Range/Stream/RawWrite.h"
#include "Range/Generators/Count.h"

#include "Container/Operations/Info.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename O> class GenericBinarySerializer
{
public:
	template<typename... Args> GenericBinarySerializer(Args&&... output):
		Output(Cpp::Forward<Args>(output)...) {}

	//! Сериализовать массив простым копированием байт.
	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_
	> SerializeArray(CSpan<T> v)
	{
		Range::WriteRaw<uintLE>(Output, uint(v.Length()));
		Range::CopyToRawAdvance(v, Output);
	}

	//! Сериализовать массив поэлементно.
	template<typename T> forceinline Meta::EnableIf<
		!Meta::IsTriviallySerializable<T>::_
	> SerializeArray(CSpan<T> v) {SerializeRange(v);}

	//! Сериализовать диапазон поэлементно.
	template<typename R> void SerializeRange(R&& v)
	{
		Range::WriteRaw<uintLE>(Output, uint(Range::Count(v)));
		Range::ForEach(Cpp::Forward<R>(v), *this);
	}


	//! Сериализация для тривиально сериализуемых типов.
	template<typename T> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_ &&
		!Concepts::IsInputRange<T>::_,
	GenericBinarySerializer&> operator<<(const T& v)
	{
		Range::WriteRaw<T>(Output, v);
		return *this;
	}

	//! Сериализация диапазонов, представленных непрерывно в памяти.
	template<typename R> forceinline Meta::EnableIf<
		Concepts::IsArrayClass<R>::_ &&
		!Concepts::IsStaticArrayContainer<R>::_,
	GenericBinarySerializer&> operator<<(R&& v)
	{
		SerializeArray(CSpanOf(v));
		return *this;
	}


	//! Сериализовать нетривиально сериализуемую структуру или класс со статической рефлексией.
	template<typename T> forceinline Meta::EnableIf<
		!Concepts::IsAsInputRange<T>::_ &&
		Meta::HasForEachField<const T&, GenericBinarySerializer&>::_ &&
		!Meta::IsTriviallySerializable<T>::_,
	GenericBinarySerializer&> operator<<(const T& src)
	{
		Meta::ForEachField(src, *this);
		return *this;
	}

	//! Сериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
	template<typename T, size_t N> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	GenericBinarySerializer&> operator<<(T(&src)[N])
	{
		Range::CopyToRawAdvance(src, Output, sizeof(T)*N/sizeof(Concepts::ValueTypeOf<O>));
		return *this;
	}

	//! Поэлементно сериализовать массив фиксированной длины нетривиально сериализуемого типа.
	template<typename T, size_t N> forceinline Meta::EnableIf<
		!Meta::IsTriviallySerializable<T>::_,
	GenericBinarySerializer&> operator<<(T(&src)[N])
	{
		Range::ForEach(src, *this);
		return *this;
	}

	//! Сериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
	template<typename C, typename T=Concepts::ElementTypeOfArray<C>> forceinline Meta::EnableIf<
		Concepts::IsStaticArrayContainer<C>::_ &&
		Meta::IsTriviallySerializable<T>::_,
	GenericBinarySerializer&> operator<<(C&& src)
	{
		Range::CopyToRawAdvance(SpanOf(src), Output);
		return *this;
	}

	//! Поэлементно сериализовать массив фиксированной длины нетривиально сериализуемого типа.
	template<typename C, typename T=Concepts::ValueTypeOf<C>> forceinline Meta::EnableIf<
		Concepts::IsStaticArrayContainer<C>::_ &&
		!Meta::IsTriviallySerializable<T>::_,
	GenericBinarySerializer&> operator<<(C&& src)
	{
		Range::ForEach(src, *this);
		return *this;
	}


	//! Сериализовать любое значение. Благодаря этому оператору класс может вести себя как функтор.
	template<typename T> forceinline GenericBinarySerializer& operator()(T&& value)
	{return *this << Cpp::Forward<T>(value);}

	//! Сериализовать любое значение.
	//! Рекомендуется использовать этот метод вместо операторов () или << и явно указывать тип.
	template<typename T> forceinline GenericBinarySerializer& Serialize(T&& value)
	{return *this << Cpp::Forward<T>(value);}

	//! Получить размер в байтах, который займёт элемент value при сериализации без учёта выравнивания.
	template<typename T> static size_t SerializedSizeOf(T&& value)
	{
		GenericBinarySerializer<Range::CountRange<byte>> dummy({});
		dummy << Cpp::Forward<T>(value);
		return dummy.Output.Counter;
	}

	O Output;
};
typedef GenericBinarySerializer<Range::OutputArrayRange<byte>> BinarySerializer;
typedef GenericBinarySerializer<Range::CountRange<byte>> DummyBinarySerializer;

INTRA_WARNING_POP

}}
