#pragma once


#include "Intra/Core.h"
#include "Intra/EachField.h"

#include "Intra/Reflection.h"

#include "Intra/Range/Operations.h"
#include "Intra/Range/Stream/RawRead.h"

#include "IntraX/Utils/Endianess.h"
#include "IntraX/Container/Operations/Append.h"

namespace Intra { INTRA_BEGIN
class BinaryDeserializer
{
public:
	/// When deserializer has no enough data to read, it calls this callback.
	/// The callback is expected to return Span with more space (at least 16 bytes).
	/// Possible implementations:
	///  1. Load the buffer from a file/socket (may be blocking in case of I/O)
	///  2. Switch to another fiber to be able to continue deserialization later (e.g. when socket becomes readable)
	///  3. Throw an exception on I/O error or unexpected EOF.
	using RequestDataCallback = PCallable<Span<const char>()>;

	explicit BinaryDeserializer(Span<const char> input, RequestDataCallback requestData):
		Input(input), RequestData(INTRA_MOVE(requestData)) {}

	/// Deserialize `count` elements and put them into dst.
	template<COutput OR> void DeserializeToOutputRange(OR& dst, size_t count)
	{
		using T = TRangeValue<OR>;
		while(count--)
			dst.Put(Deserialize<T>());
	}

	/// Deserialize trivially serializable types.
	template<CTriviallySerializable T> requires (!CRange<T>)
	BinaryDeserializer& operator>>(T& dst)
	{
		RawRead(Input, dst);
		return *this;
	}

	/// Deserialize GenericStringView referring to the data from Input.
	template<typename Char> BinaryDeserializer& operator>>(GenericStringView<Char>& dst)
	{
		Span<const Char> result;
		*this >> result;
		dst = GenericStringView<Char>(result);
		return *this;
	}

	/// Deserialize a container.
	template<CList L> requires (!CRange<L>) BinaryDeserializer& operator>>(L& dst)
	{
		using T = TListValue<L>;
		uint32 len = Deserialize<uint32LE>();
		if constexpr(CDynamicArrayContainer<L> && CTriviallySerializable<T>)
		{
			dst.resize(len);
			Span<T> dstArr(dst);
			RawReadTo(Input, dstArr);
		}
		else if constexpr(CStaticArrayContainer<L>)
		{
			if constexpr(CTriviallySerializable<T>) RawReadTo(Input, SpanOf(dst));
			else DeserializeToOutputRange(Span(dst), Length(dst));
		}
		else if constexpr(CHas_push_back<L> && CHas_clear<L>)
		{
			dst.clear();
			Reserve(dst, count);
			auto appender = LastAppender(dst);
			DeserializeToOutputRange(appender, count);
		}
		return *this;
	}

	
	/// Deserialize an array into a CSpan.
	/// Works only for trivially serializable types, since `dst` refers to raw data inside Input.
	template<CTriviallySerializable T> BinaryDeserializer& operator>>(Span<const T>& dst)
	{
		uint32 count = Deserialize<uint32LE>();
		dst = Span<const T>(reinterpret_cast<const T*>(Input.Data()), count);
		Input|PopFirstExactly(count * sizeof(T));
		return *this;
	}

	/// Deserialize a non-trivially serializable struct or class with static reflection field information.
	template<typename T> requires CHasForEachField<T&, BinaryDeserializer&> && (!CTriviallySerializable<T>)
	BinaryDeserializer& operator>>(T& dst)
	{
		ForEachField(dst, *this);
		return *this;
	}

	/// Deserialize a fixed size array.
	template<typename T, size_t N> BinaryDeserializer& operator>>(T(&dst)[N])
	{
		if constexpr(CTriviallySerializable) RawReadTo(Input, Span<T>(dst));
		else dst|ForEach(*this);
		return *this;
	}


	/// Deserialize a value into a variable of any type.
	/// This makes the deserializer usable in algorithms as a functor.
	template<typename T> requires(!CConst<T>)
	BinaryDeserializer& operator()(T&& value) {return *this >> INTRA_FWD(value);}

	/// Deserialize a value of specified type.
	template<typename T> T Deserialize()
	{
		T result;
		*this >> result;
		return result;
	}

	Span<char> Input;
	RequestDataCallback RequestData;
};
} INTRA_END
