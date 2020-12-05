#pragma once

#include "Intra/Reflection.h"

#include "Intra/Type.h"
#include "Intra/EachField.h"

#include "Intra/Range/Span.h"

#include "Intra/CContainer.h"
#include "Intra/Range/Concepts.h"

#include "Intra/Range/Operations.h"
#include "Intra/Range/ForEach.h"

#include "Intra/Range/Stream/RawWrite.h"
#include "Intra/Range/Count.h"

#include "IntraX/Utils/Endianess.h"
#include "IntraX/Container/Operations/Info.h"

INTRA_BEGIN
template<typename O> class GenericBinarySerializer
{
public:
	template<typename... Args> GenericBinarySerializer(Args&&... output):
		Output(Forward<Args>(output)...) {}

	/// Serialize array.
	template<typename T> Requires<
		CTriviallySerializable<T>
	> SerializeArray(CSpan<T> v)
	{
		RawWrite<uint32LE>(Output, unsigned(v.Length()));
		RawWriteFrom(Output, v);
	}
	template<typename T> Requires<
		!CTriviallySerializable<T>
	> SerializeArray(CSpan<T> v) {SerializeRange(v);}

	/// Serialize range.
	template<typename R> void SerializeRange(R&& v)
	{
		RawWrite<uint32LE>(Output, unsigned(Count(v)));
		ForEach(Forward<R>(v), *this);
	}


	/// Serialize anything. To avoid unintended type changes it is recommended to use Serialize method instead with an explicitly specified type.
	template<typename T> Requires<
		CTriviallySerializable<T> &&
		!CRange<T>,
	GenericBinarySerializer&> operator<<(const T& v)
	{
		RawWrite<T>(Output, v);
		return *this;
	}
	template<typename R> Requires<
		CArrayList<R> &&
		!CStaticArrayContainer<R>,
	GenericBinarySerializer&> operator<<(R&& v)
	{
		SerializeArray(CSpanOf(v));
		return *this;
	}
	template<typename T> Requires<
		!CList<T> &&
		CHasForEachField<const T&, GenericBinarySerializer&> &&
		!CTriviallySerializable<T>,
	GenericBinarySerializer&> operator<<(const T& src)
	{
		ForEachField(src, *this);
		return *this;
	}

#if 0 //delete this if raw array serialization works without this code
	template<typename T, size_t N> Requires<
		CTriviallySerializable<T>,
	GenericBinarySerializer&> operator<<(T(&src)[N])
	{
		RawWriteFrom(Output, src, sizeof(T)*N);
		return *this;
	}
	template<typename T, size_t N> Requires<
		!CTriviallySerializable<T>,
	GenericBinarySerializer&> operator<<(T(&src)[N])
	{
		ForEach(src, *this);
		return *this;
	}
#endif

	template<typename C, typename T=TArrayElement<C>> Requires<
		CStaticArrayContainer<C> &&
		CTriviallySerializable<T>,
	GenericBinarySerializer&> operator<<(C&& src)
	{
		RawWriteFrom(Output, SpanOf(src));
		return *this;
	}
	template<typename C, typename T=TRangeValue<C>> Requires<
		CStaticArrayContainer<C> &&
		!CTriviallySerializable<T>,
	GenericBinarySerializer&> operator<<(C&& src)
	{
		ForEach(src, *this);
		return *this;
	}


	/// Serialize anything. This operator makes this class functor.
	template<typename T> GenericBinarySerializer& operator()(T&& value)
	{return *this << Forward<T>(value);}

	/// Serialize anything.
	/// It is recommended to use this operator instead of () or << and specify the type explicitly.
	template<typename T> GenericBinarySerializer& Serialize(const T& value)
	{return *this << value;}

	/// Calculate serialized size of a value without doing any serialization.
	template<typename T> static size_t SerializedSizeOf(T&& value)
	{
		GenericBinarySerializer<CountRange<byte>> dummy({});
		dummy << Forward<T>(value);
		return dummy.Output.Counter;
	}

	O Output;
};
using BinarySerializer = GenericBinarySerializer<SpanOutput<byte>>;
using DummyBinarySerializer = GenericBinarySerializer<CountRange<byte>>;
INTRA_END
