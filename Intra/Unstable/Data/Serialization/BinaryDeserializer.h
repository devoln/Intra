#pragma once


#include "Core/Type.h"
#include "Core/EachField.h"

#include "Core/Reflection.h"

#include "Core/Range/Operations.h"
#include "Core/Range/Stream/RawRead.h"

#include "Utils/Endianess.h"
#include "Container/Operations/Append.h"

INTRA_BEGIN
template<typename I> class GenericBinaryDeserializer
{
public:
	GenericBinaryDeserializer(const I& input): Input(input) {}
	GenericBinaryDeserializer(I&& input): Input(Move(input)) {}

	//! Deserialize `count` elements and put them into dst.
	template<typename OR> Requires<
		COutputRange<OR>
	> DeserializeToOutputRange(OR& dst, size_t count)
	{
		typedef TValueTypeOf<OR> T;
		while(count --> 0)
			dst.Put(Deserialize<T>());
	}


	//! Deserialize trivially serializable types.
	template<typename T> forceinline Requires<
		CPod<T> &&
		!CInputRange<T>,
	GenericBinaryDeserializer&> operator>>(T& dst)
	{
		RawRead(Input, dst);
		return *this;
	}

	//! Deserialize GenericStringView referring to the data from Input.
	template<typename Char> GenericBinaryDeserializer& operator>>(GenericStringView<const Char>& dst)
	{
		CSpan<Char> result;
		*this >> result;
		dst = GenericStringView<const Char>(result);
		return *this;
	}

	//! Deserialize a dynamic array container of trivially serializable type.
	template<typename C,
		typename T = TValueTypeOf<C>
	> Requires<
		CPod<T> &&
		CDynamicArrayContainer<C>,
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		uint len = Deserialize<uint32LE>();
		dst.resize(len);
		Span<T> dstArr(dst.data(), dst.size());
		RawReadTo(Input, dstArr);
		return *this;
	}

	//! Deserialize a container that is not a dynamic array of trivially serializable type.
	template<typename C> Requires<
		CHas_push_back<C> && CHas_clear<C> &&
		!(CDynamicArrayContainer<C> &&
			CPod<TValueTypeOf<C>>),
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		uint count = Deserialize<uint32LE>();
		dst.clear();
		Reserve(dst, count);
		auto appender = LastAppender(dst);
		DeserializeToOutputRange(appender, count);
		return *this;
	}

	//! Deserialize a static array container of trivially serializable type.
	template<typename C, typename T = TValueTypeOf<C>> Requires<
		CPod<T> &&
		CStaticArrayContainer<C>,
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		Span<T> dstArr(dst.data(), dst.size());
		RawReadTo(Input, dstArr);
		return *this;
	}

	//! Deserialize a static array container of non-trivially serializable type.
	template<typename C,
		typename T = TValueTypeOf<C>
	> Requires<
		!CPod<T> &&
		CStaticArrayContainer<C>,
	GenericBinaryDeserializer&> operator>>(C& dst)
	{
		Span<T> dstArr(dst.data(), dst.size());
		DeserializeToOutputRange(dstArr, dstArr.Length());
		return *this;
	}

	
	//! Deserialize an array into a CSpan.
	//! Works only for trivially serializable types,
	//! since `dst` refers to raw data inside Input.
	template<typename T> Requires<
		CPod<T>,
	GenericBinaryDeserializer&> operator>>(CSpan<T>& dst)
	{
		uint count = Deserialize<uint32LE>();
		dst = CSpan<T>(reinterpret_cast<const T*>(Input.Data()), count);
		PopFirstExactly(Input, count*sizeof(T));
		return *this;
	}

	//! Deserialize a non-trivially serializable struct or class with static reflection field information.
	template<typename T> Requires<
		CHasForEachField<T&, GenericBinaryDeserializer&> &&
		!CPod<T>,
	GenericBinaryDeserializer&> operator>>(T& dst)
	{
		ForEachField(dst, *this);
		return *this;
	}

	//! Deserialize a fixed size array of a trivially serializable type.
	template<typename T, size_t N> Requires<
		CPod<T>,
	GenericBinaryDeserializer&> operator>>(T(&dst)[N])
	{
		RawReadTo(Input, Span<T>(dst));
		return *this;
	}

	//! Deserialize a fixed size array of a non-trivially serializable type.
	template<typename T, size_t N> Requires<
		!CPod<T>,
	GenericBinaryDeserializer&> operator>>(T(&dst)[N])
	{
		ForEach(dst, *this);
		return *this;
	}


	//! Deserialize a value into a variable of any type.
	//! This makes the deserializer usable in algorithms as a functor.
	template<typename T> forceinline Requires<
		!CConst<T>,
	GenericBinaryDeserializer&> operator()(T&& value)
	{return *this >> Forward<T>(value);}

	//! Deserialize a value of specified type.
	template<typename T> forceinline T Deserialize()
	{
		T result;
		*this >> result;
		return result;
	}

	I Input;
};
typedef GenericBinaryDeserializer<Span<byte>> BinaryDeserializer;
INTRA_END
