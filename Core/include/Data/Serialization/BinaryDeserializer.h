#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/Endianess.h"
#include "Meta/Preprocessor.h"
#include "Meta/Type.h"
#include "Meta/EachField.h"
#include "Data/Reflection.h"
#include "Range/Operations.h"
#include "Algo/Raw/Read.h"
#include "Container/Operations/Append.h"

namespace Intra { namespace Data {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename I> class GenericBinaryDeserializer
{
public:
	GenericBinaryDeserializer(const I& input): Input(input) {}
	GenericBinaryDeserializer(I&& input): Input(Meta::Move(input)) {}

	//! Поэлементно десериализовать массив длиной count в OutputRange dst.
	template<typename OR> Meta::EnableIf<
		Range::IsOutputRange<OR>::_
	> DeserializeToOutputRange(OR& dst, size_t count)
	{
		typedef Range::ValueTypeOf<OR> T;
		while(count --> 0)
			dst.Put(Deserialize<T>());
	}


	//! Десериализация для тривиально сериализуемых типов.
	template<typename T> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_ && !Range::IsInputRange<T>::_,
	GenericBinaryDeserializer&> operator>>(T& dst)
	{
		Algo::CopyAdvanceToRawOne(Input, dst);
		return *this;
	}

	//! Десериализация GenericStringView. Результат ссылается на данные в сериализованной области.
	template<typename Char> GenericBinaryDeserializer& operator>>(GenericStringView<const Char>& dst)
	{
		CSpan<Char> result;
		*this >> result;
		dst = {result.Begin, result.End};
		return *this;
	}

	//! Десериализация контейнера, который является ДИНАМИЧЕСКИМ массивом тривиально сериализуемого типа.
	template<typename C, typename T=Range::ValueTypeOf<C>> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_ &&
		Container::IsDynamicArrayContainer<C>::_,
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		uint len = Deserialize<uintLE>();
		dst.resize(len);
		Span<T> dstArr(Container::Data(dst), dst.size());
		Algo::CopyAdvanceToRaw(Input, dstArr);
		return *this;
	}

	//! Десериализация контейнера, который НЕ является массивом тривиально сериализуемого типа.
	template<typename C> Meta::EnableIf<
		Container::Has_push_back<C>::_ && Container::Has_clear<C>::_ &&
		!(Container::IsDynamicArrayContainer<C>::_ &&
			Meta::IsTriviallySerializable<Range::ValueTypeOf<C>>::_),
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		uint count = Deserialize<uintLE>();
		dst.clear();
		Container::Reserve(dst, count);
		auto appender = Range::LastAppender(dst);
		DeserializeToOutputRange(appender, count);
		return *this;
	}

	//! Десериализация контейнера, который является СТАТИЧЕСКИМ массивом тривиально сериализуемого типа.
	template<typename C, typename T = Container::ValueTypeOf<C>> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_ &&
		Container::IsStaticArrayContainer<C>::_,
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		Span<T> dstArr(Container::Data(dst), dst.size());
		Algo::CopyAdvanceToRaw(Input, dstArr);
		return *this;
	}

	//! Десериализация контейнера, который является СТАТИЧЕСКИМ массивом НЕтривиально сериализуемого типа.
	template<typename C, typename T = Container::ValueTypeOf<C>> Meta::EnableIf<
		!Meta::IsTriviallySerializable<T>::_ &&
		Container::IsStaticArrayContainer<C>::_,
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		Span<T> dstArr(Container::Data(dst), dst.size());
		DeserializeToOutputRange(dstArr, dstArr.Length());
		return *this;
	}

	
	//! Десериализовать массив в Span.
	//! Это можно делать только для тривиально сериализуемых типов,
	//! так как полученный dst ссылается на данные в сериализованной области.
	template<typename T> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	GenericBinaryDeserializer&> operator>>(CSpan<T>& dst)
	{
		uint count = Deserialize<uintLE>();
		dst = CSpan<T>(reinterpret_cast<const T*>(Input.Data()), count);
		Range::PopFirstExactly(Input, count*sizeof(T));
		return *this;
	}

	//! Десериализовать нетривиально сериализуемую структуру или класс со статической рефлексией.
	template<typename T> Meta::EnableIf<
		Meta::HasForEachField<T&, GenericBinaryDeserializer&>::_ &&
		!Meta::IsTriviallySerializable<T>::_,
	GenericBinaryDeserializer&> operator>>(T& dst)
	{
		Meta::ForEachField(dst, *this);
		return *this;
	}

	//! Десериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
	template<typename T, size_t N> Meta::EnableIf<
		Meta::IsTriviallySerializable<T>::_,
	GenericBinaryDeserializer&> operator>>(T(&dst)[N])
	{
		Algo::CopyAdvanceToRaw(Input, Span<T>(dst));
		return *this;
	}

	//! Поэлементно десериализовать массив фиксированной длины нетривально десериализуемого типа.
	template<typename T, size_t N> Meta::EnableIf<
		!Meta::IsTriviallySerializable<T>::_,
	GenericBinaryDeserializer&> operator>>(T(&dst)[N])
	{
		Algo::ForEach(dst, *this);
		return *this;
	}


	//! Десериализовать любое значение по ссылке на него.
	//! Этот оператор нужен, чтобы использовать десериализатор как функтор и передавать в алгоритмы.
	template<typename T> forceinline Meta::EnableIf<
		!Meta::IsConst<T>::_,
	GenericBinaryDeserializer&> operator()(T&& value)
	{return *this >> Meta::Forward<T>(value);}

	//! Десериализовать любое значение и вернуть его из метода.
	template<typename T> forceinline T Deserialize()
	{
		T result;
		*this >> result;
		return result;
	}

	I Input;
};

typedef GenericBinaryDeserializer<Span<byte>> BinaryDeserializer;


//! Десериализовать runtime структуру
//void DeserializeStructBinary(BinaryDeserializer& deserializer, const StructReflection& reflection, void*& dst);


INTRA_WARNING_POP

}}
