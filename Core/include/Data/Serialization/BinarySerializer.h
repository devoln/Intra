#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/Endianess.h"
#include "Meta/Preprocessor.h"
#include "Meta/Type.h"
#include "Meta/EachField.h"
#include "Data/Reflection.h"
#include "Range/Operations.h"
#include "Algo/ForEach.h"
#include "Range/Output/OutputArrayRange.h"
#include "Algo/Raw/Write.h"
#include "Range/Generators/Count.h"
#include "Container/Operations/Info.h"
#include "Container/Operations/Extension.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename O> class GenericBinarySerializer
{
public:
	template<typename... Args> GenericBinarySerializer(Args&&... output):
		Output(Meta::Forward<Args>(output)...) {}

	//! Сериализовать массив простым копированием байт.
	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_
	> SerializeArray(CSpan<T> v)
	{
		Algo::WriteRaw<uintLE>(Output, uint(v.Length()));
		Algo::CopyToRawAdvance(v, Output);
	}

	//! Сериализовать массив поэлементно.
	template<typename T> forceinline Meta::EnableIf<
		!Meta::IsTriviallySerializable<T>::_
	> SerializeArray(CSpan<T> v) {SerializeRange(v);}

	//! Сериализовать диапазон поэлементно.
	template<typename R> void SerializeRange(R&& v)
	{
		Algo::WriteRaw<uintLE>(Output, uint(Range::Count(v)));
		Algo::ForEach(Meta::Forward<R>(v), *this);
	}


	//! Сериализация для тривиально сериализуемых типов.
	template<typename T> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_ && !Range::IsInputRange<T>::_,
	GenericBinarySerializer&> operator<<(const T& v)
	{
		Algo::WriteRaw<T>(Output, v);
		return *this;
	}

	//! Сериализация диапазонов, представленных непрерывно в памяти.
	template<typename R> forceinline Meta::EnableIf<
		Range::IsAsArrayRange<R>::_ &&
		!Container::IsStaticArrayContainer<R>::_,
	GenericBinarySerializer&> operator<<(R&& v)
	{
		auto range = Range::Forward<R>(v);
		typedef Range::ValueTypeOfAs<R> T;
		SerializeArray(CSpan<T>(range.Data(), range.Length()));
		return *this;
	}


	//! Сериализовать нетривиально сериализуемую структуру или класс со статической рефлексией.
	template<typename T> forceinline Meta::EnableIf<
		!Range::IsAsInputRange<T>::_ &&
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
		Algo::CopyToRawAdvance(src, Output, sizeof(T)*N/sizeof(Range::ValueTypeOf<O>));
		return *this;
	}

	//! Поэлементно сериализовать массив фиксированной длины нетривиально сериализуемого типа.
	template<typename T, size_t N> forceinline Meta::EnableIf<
		!Meta::IsTriviallySerializable<T>::_,
	GenericBinarySerializer&> operator<<(T(&src)[N])
	{
		Algo::ForEach(src, *this);
		return *this;
	}

	//! Сериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
	template<typename C, typename T= Container::ValueTypeOf<C>> forceinline Meta::EnableIf<
		Container::IsStaticArrayContainer<C>::_ &&
		Meta::IsTriviallySerializable<T>::_,
	GenericBinarySerializer&> operator<<(C&& src)
	{
		Algo::CopyToRawAdvance(Span<T>(src), Output);
		return *this;
	}

	//! Поэлементно сериализовать массив фиксированной длины нетривиально сериализуемого типа.
	template<typename C, typename T=Container::ValueTypeOf<C>> forceinline Meta::EnableIf<
		Container::IsStaticArrayContainer<C>::_ &&
		!Meta::IsTriviallySerializable<T>::_,
	GenericBinarySerializer&> operator<<(C&& src)
	{
		Algo::ForEach(src, *this);
		return *this;
	}


	//! Сериализовать любое значение. Благодаря этому оператору класс может вести себя как функтор.
	template<typename T> forceinline GenericBinarySerializer& operator()(T&& value)
	{return *this << Meta::Forward<T>(value);}

	//! Сериализовать любое значение.
	//! Рекомендуется использовать этот метод вместо операторов () или << и явно указывать тип.
	template<typename T> forceinline GenericBinarySerializer& Serialize(T&& value)
	{return *this << Meta::Forward<T>(value);}

	//! Получить размер в байтах, который займёт элемент value при сериализации без учёта выравнивания.
	template<typename T> static size_t SerializedSizeOf(T&& value)
	{
		GenericBinarySerializer<Range::CountRange<byte>> dummy({});
		dummy << Meta::Forward<T>(value);
		return dummy.Output.Counter;
	}

	O Output;
};
typedef GenericBinarySerializer<Range::OutputArrayRange<byte>> BinarySerializer;
typedef GenericBinarySerializer<Range::CountRange<byte>> DummyBinarySerializer;

INTRA_WARNING_POP

}}
