#pragma once


#include "Core/Endianess.h"

#include "Data/Reflection.h"

#include "Core/Type.h"
#include "Core/EachField.h"

#include "Core/Range/Span.h"

#include "Core/Container.h"
#include "Core/Range/Concepts.h"

#include "Core/Range/Operations.h"
#include "Core/Range/ForEach.h"

#include "Core/Range/Stream/RawWrite.h"
#include "Core/Range/Generators/Count.h"

#include "Container/Operations/Info.h"

INTRA_BEGIN
namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename O> class GenericBinarySerializer
{
public:
	template<typename... Args> GenericBinarySerializer(Args&&... output):
		Output(Forward<Args>(output)...) {}

	//! Сериализовать массив простым копированием байт.
	template<typename T> Requires<
		CPod<T>::_
	> SerializeArray(CSpan<T> v)
	{
		Range::RawWrite<uintLE>(Output, uint(v.Length()));
		Range::RawWriteFrom(Output, v);
	}

	//! Сериализовать массив поэлементно.
	template<typename T> forceinline Requires<
		!CPod<T>::_
	> SerializeArray(CSpan<T> v) {SerializeRange(v);}

	//! Сериализовать диапазон поэлементно.
	template<typename R> void SerializeRange(R&& v)
	{
		Range::RawWrite<uintLE>(Output, uint(Range::Count(v)));
		Range::ForEach(Forward<R>(v), *this);
	}


	//! Сериализация для тривиально сериализуемых типов.
	template<typename T> forceinline Requires<
		CPod<T>::_ &&
		!CInputRange<T>::_,
	GenericBinarySerializer&> operator<<(const T& v)
	{
		Range::RawWrite<T>(Output, v);
		return *this;
	}

	//! Сериализация диапазонов, представленных непрерывно в памяти.
	template<typename R> forceinline Requires<
		CArrayClass<R>::_ &&
		!CStaticArrayContainer<R>::_,
	GenericBinarySerializer&> operator<<(R&& v)
	{
		SerializeArray(CSpanOf(v));
		return *this;
	}


	//! Сериализовать нетривиально сериализуемую структуру или класс со статической рефлексией.
	template<typename T> forceinline Requires<
		!CAsInputRange<T>::_ &&
		CHasForEachField<const T&, GenericBinarySerializer&>::_ &&
		!CPod<T>::_,
	GenericBinarySerializer&> operator<<(const T& src)
	{
		Core::ForEachField(src, *this);
		return *this;
	}

	//! Сериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
	template<typename T, size_t N> forceinline Requires<
		CPod<T>::_,
	GenericBinarySerializer&> operator<<(T(&src)[N])
	{
		Range::RawWriteFrom(Output, src, sizeof(T)*N);
		return *this;
	}

	//! Поэлементно сериализовать массив фиксированной длины нетривиально сериализуемого типа.
	template<typename T, size_t N> forceinline Requires<
		!CPod<T>::_,
	GenericBinarySerializer&> operator<<(T(&src)[N])
	{
		Range::ForEach(src, *this);
		return *this;
	}

	//! Сериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
	template<typename C, typename T=TArrayElement<C>> forceinline Requires<
		CStaticArrayContainer<C>::_ &&
		CPod<T>::_,
	GenericBinarySerializer&> operator<<(C&& src)
	{
		Range::RawWriteFrom(Output, SpanOf(src));
		return *this;
	}

	//! Поэлементно сериализовать массив фиксированной длины нетривиально сериализуемого типа.
	template<typename C, typename T=TValueTypeOf<C>> forceinline Requires<
		CStaticArrayContainer<C>::_ &&
		!CPod<T>::_,
	GenericBinarySerializer&> operator<<(C&& src)
	{
		Range::ForEach(src, *this);
		return *this;
	}


	//! Сериализовать любое значение. Благодаря этому оператору класс может вести себя как функтор.
	template<typename T> forceinline GenericBinarySerializer& operator()(T&& value)
	{return *this << Forward<T>(value);}

	//! Сериализовать любое значение.
	//! Рекомендуется использовать этот метод вместо операторов () или << и явно указывать тип.
	template<typename T> forceinline GenericBinarySerializer& Serialize(T&& value)
	{return *this << Forward<T>(value);}

	//! Получить размер в байтах, который займёт элемент value при сериализации без учёта выравнивания.
	template<typename T> static size_t SerializedSizeOf(T&& value)
	{
		GenericBinarySerializer<CountRange<byte>> dummy({});
		dummy << Forward<T>(value);
		return dummy.Output.Counter;
	}

	O Output;
};
typedef GenericBinarySerializer<OutputArrayRange<byte>> BinarySerializer;
typedef GenericBinarySerializer<CountRange<byte>> DummyBinarySerializer;

INTRA_WARNING_POP

}}
