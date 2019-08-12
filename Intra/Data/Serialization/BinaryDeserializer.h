#pragma once


#include "Core/Endianess.h"

#include "Core/Type.h"
#include "Core/EachField.h"

#include "Data/Reflection.h"

#include "Core/Range/Operations.h"
#include "Core/Range/Stream/RawRead.h"

#include "Container/Operations/Append.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace Data {

template<typename I> class GenericBinaryDeserializer
{
public:
	GenericBinaryDeserializer(const I& input): Input(input) {}
	GenericBinaryDeserializer(I&& input): Input(Move(input)) {}

	//! Поэлементно десериализовать массив длиной count в OutputRange dst.
	template<typename OR> Requires<
		COutputRange<OR>::_
	> DeserializeToOutputRange(OR& dst, size_t count)
	{
		typedef TValueTypeOf<OR> T;
		while(count --> 0)
			dst.Put(Deserialize<T>());
	}


	//! Десериализация для тривиально сериализуемых типов.
	template<typename T> forceinline Requires<
		CPod<T>::_ &&
		!CInputRange<T>::_,
	GenericBinaryDeserializer&> operator>>(T& dst)
	{
		Range::RawRead(Input, dst);
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
	template<typename C,
		typename T = TValueTypeOf<C>
	> Requires<
		CPod<T>::_ &&
		CDynamicArrayContainer<C>::_,
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		uint len = Deserialize<uintLE>();
		dst.resize(len);
		Span<T> dstArr(dst.data(), dst.size());
		Range::RawReadTo(Input, dstArr);
		return *this;
	}

	//! Десериализация контейнера, который НЕ является массивом тривиально сериализуемого типа.
	template<typename C> Requires<
		CHas_push_back<C>::_ && CHas_clear<C>::_ &&
		!(CDynamicArrayContainer<C>::_ &&
			CPod<TValueTypeOf<C>>::_),
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		uint count = Deserialize<uintLE>();
		dst.clear();
		Core::Reserve(dst, count);
		auto appender = Range::LastAppender(dst);
		DeserializeToOutputRange(appender, count);
		return *this;
	}

	//! Десериализация контейнера, который является СТАТИЧЕСКИМ массивом тривиально сериализуемого типа.
	template<typename C, typename T = TValueTypeOf<C>> Requires<
		CPod<T>::_ &&
		CStaticArrayContainer<C>::_,
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		Span<T> dstArr(dst.data(), dst.size());
		Range::RawReadTo(Input, dstArr);
		return *this;
	}

	//! Десериализация контейнера, который является СТАТИЧЕСКИМ массивом НЕтривиально сериализуемого типа.
	template<typename C,
		typename T = TValueTypeOf<C>
	> Requires<
		!CPod<T>::_ &&
		CStaticArrayContainer<C>::_,
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		Span<T> dstArr(dst.data(), dst.size());
		DeserializeToOutputRange(dstArr, dstArr.Length());
		return *this;
	}

	
	//! Десериализовать массив в Span.
	//! Это можно делать только для тривиально сериализуемых типов,
	//! так как полученный dst ссылается на данные в сериализованной области.
	template<typename T> Requires<
		CPod<T>::_,
	GenericBinaryDeserializer&> operator>>(CSpan<T>& dst)
	{
		uint count = Deserialize<uintLE>();
		dst = CSpan<T>(reinterpret_cast<const T*>(Input.Data()), count);
		Range::PopFirstExactly(Input, count*sizeof(T));
		return *this;
	}

	//! Десериализовать нетривиально сериализуемую структуру или класс со статической рефлексией.
	template<typename T> Requires<
		CHasForEachField<T&, GenericBinaryDeserializer&>::_ &&
		!CPod<T>::_,
	GenericBinaryDeserializer&> operator>>(T& dst)
	{
		Core::ForEachField(dst, *this);
		return *this;
	}

	//! Десериализовать массив фиксированной длины тривиально сериализуемого типа побайтовым копированием.
	template<typename T, size_t N> Requires<
		CPod<T>::_,
	GenericBinaryDeserializer&> operator>>(T(&dst)[N])
	{
		Range::RawReadTo(Input, Span<T>(dst));
		return *this;
	}

	//! Поэлементно десериализовать массив фиксированной длины нетривально десериализуемого типа.
	template<typename T, size_t N> Requires<
		!CPod<T>::_,
	GenericBinaryDeserializer&> operator>>(T(&dst)[N])
	{
		Range::ForEach(dst, *this);
		return *this;
	}


	//! Десериализовать любое значение по ссылке на него.
	//! Этот оператор нужен, чтобы использовать десериализатор как функтор и передавать в алгоритмы.
	template<typename T> forceinline Requires<
		!CConst<T>::_,
	GenericBinaryDeserializer&> operator()(T&& value)
	{return *this >> Forward<T>(value);}

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


}}

INTRA_WARNING_POP
