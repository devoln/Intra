#pragma once

#include <Intra/Concepts.h>
#include <Intra/TypeErasure.h>
#include <Intra/Container/Compound.h>
#include <Intra/Range.h>

namespace Intra { INTRA_BEGIN
class BinarySerializer
{
public:
	/// When serializer has no enough space to write, it calls this callback.
	/// The callback is expected to return Span with more space (at least 16 bytes).
	/// Possible implementations:
	///  1. Resize the container receiving serialized data
	///  2. Flush the buffer to a file/socket to allow filling it again (may be blocking in case of I/O)
	///  3. Switch to another fiber to be able to continue serialization later (e.g. when socket becomes writable)
	///  4. Throw an exception if it's impossible to grow the buffer
	using FlushCallback = PCallable<Span<char>()>;

	explicit BinarySerializer(Span<char> output, FlushCallback flush):
		Output(output), Flush(INTRA_MOVE(flush)) {}

	void SerializeLength(uint64 len)
	{
		if(FreeBufferSpace() < 9) Output = RequestMoreSpace();
		Output|PopFirstExactly(EncodeP8UintLEUnsafe(size_t(len), Output.Begin));
	}

	LResult<size_t> Write(Span<const char> src)
	{
		write(src);
		return src.Length();
	}

	template<CList L> void SerializeListWithoutLength(L&& list)
	{
		if constexpr(CConvertibleToSpan<L>) if constexpr(CTriviallySerializable<TListValue<L>>)
		{
			writeRaw(Span(list));
			return;
		}
		INTRA_FWD(list)|ForEach(*this);
	}

	template<CList L> void SerializeList(L&& list)
	{
		SerializeLength(list|Count);
		SerializeListWithoutLength(INTRA_FWD(list));
	}

	/// Serialize anything. To avoid unintended type changes it is recommended to use Serialize method with an explicitly specified type instead.
	template<typename T> requires CList<T> || CStaticLengthContainer<T> || CTriviallySerializable<T>
	BinarySerializer& operator<<(T&& v)
	{
		if constexpr(CList<T>) SerializeList(INTRA_FWD(v));
		else if constexpr(CStaticLengthContainer<T>) ForEachField(*this)(v);
		else if(CTriviallySerializable<T>) writeRaw(Span(Unsafe, &v, 1));
		else static_assert(CFalse<T>);
		return *this;
	}

	/// Serialize anything. This operator makes this class functor.
	template<typename T> auto operator()(T&& value) -> decltype(*this << INTRA_FWD(value)) {return *this << INTRA_FWD(value);}

	/// Serialize anything.
	/// It is recommended to use this method instead of () or << and specify the type explicitly.
	template<typename T> BinarySerializer& Serialize(const TExplicitType<T>& value) {return *this << value;}

	Span<char> Output;
	FlushCallback Flush;

private:
	INTRA_NOINLINE void write(Span<const char> src)
	{
		for(;;)
		{
			Advance(src)|WriteTo(Output);
			if(src.Empty()) break;
			Output = Flush();
		}
	}

	template<CTriviallySerializable T> INTRA_FORCEINLINE void writeRaw(Span<const T> src)
	{
		write(Span<const char>(Unsafe, reinterpret_cast<const char*>(src.Begin), reinterpret_cast<const char*>(src.End)));
	}
};
} INTRA_END
