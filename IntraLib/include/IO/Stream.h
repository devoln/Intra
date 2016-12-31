#pragma once

#include "Data/BinarySerialization.h"
#include "Meta/Type.h"
#include "Containers/String.h"

namespace Intra {

namespace IO {

struct endl_t {};
extern endl_t endl;

}
forceinline StringView ToString(IO::endl_t) {return "\r\n";}

namespace IO
{
class IInputStream
{
public:
	virtual ~IInputStream() {}

	virtual bool EndOfStream() const=0;
	virtual size_t ReadData(void* data, size_t bytes)=0;
	virtual void UnreadData(const void* data, size_t bytes)=0;

	String ReadToChar(const AsciiSet& stopCharset, char* oStopChar=null);

	String ReadLine(bool consumeCRLF=true)
	{
		char stopChar='\0';
		String result = ReadToChar("\n\r", &stopChar);
		if(stopChar=='\n' || !consumeCRLF) return result;

		char c = Read<char>();	
		const bool endOfLineWasFullyConsumed = (c!='\n' && c!='\0');
		if(endOfLineWasFullyConsumed && !EndOfStream()) UnreadData(&c, 1);

		return result;
	}

	forceinline void SkipLine(bool consumeCRLF=true) {ReadLine(consumeCRLF);}

	forceinline String ReadCStr() {return ReadToChar("");}

	long64 ParseInteger(bool* error=null);
	real ParseFloat(bool* error=null);

	friend IInputStream& operator>>(IInputStream& stream, String& s) {s = stream.ReadLine(); return stream;}
	friend IInputStream& operator>>(IInputStream& stream, long64& n) {n = stream.ParseInteger(); return stream;}
	friend IInputStream& operator>>(IInputStream& stream, byte& n);
	friend IInputStream& operator>>(IInputStream& stream, sbyte& n);
	friend IInputStream& operator>>(IInputStream& stream, ushort& n);
	friend IInputStream& operator>>(IInputStream& stream, short& n);
	friend IInputStream& operator>>(IInputStream& stream, uint& n);
	friend IInputStream& operator>>(IInputStream& stream, int& n);
	friend IInputStream& operator>>(IInputStream& stream, real& n) {n = stream.ParseFloat(); return stream;}
	friend IInputStream& operator>>(IInputStream& stream, double& n) {n = double(stream.ParseFloat()); return stream;}
	friend IInputStream& operator>>(IInputStream& stream, float& n) {n = float(stream.ParseFloat()); return stream;}
	friend IInputStream& operator>>(IInputStream& stream, const char* r);

	forceinline void Skip(ulong64 bytes) {SetPos(GetPos()+bytes);} //Правильно работает только для потоков с переопределёнными GetPos и SetPos
	forceinline bool IsSeekable() const {return GetPos()!=~0ull;}
	virtual void SetPos(ulong64) {}
	virtual ulong64 GetSize() const {return ~0ull;} //По умолчанию поток будет считаться бесконечным
	virtual ulong64 GetPos() const {return ~0ull;} //По умолчанию у потока нет позиции. Возвращаемое значение (ulong64)-1 говорит о том, что поток не seekable


	String ReadNChars(size_t n)
	{
		String result;
		result.SetLengthUninitialized(n);
		ReadData(result.Data(), n);
		return result;
	}

	template<typename T> Meta::EnableIfPod<T, Array<T>> ReadArray(size_t count)
	{
		Array<T> result;
		result.SetCountUninitialized(count);
		ReadData(result.Data(), count*sizeof(T));
		return result;
	}

	forceinline String ReadBinString() {return ReadNChars(Read<uintLE>());}

	template<typename T> forceinline T Read() {T n; ReadData(&n, sizeof(n)); return n;}


	struct ByLineResult
	{
		enum: bool {RangeIsFinite = true};

		String CurLine;
		String Terminator;
		IInputStream* MyStream;
		bool UseTerminator;
		bool ConsumeCRLF;

		ByLineResult(null_t=null):
			CurLine(null), Terminator(null), MyStream(null),
			UseTerminator(false), ConsumeCRLF(false), mIsEmpty(true) {}

		ByLineResult(IInputStream* stream, bool consumeCRLF, bool useTerminator, const String& terminator=null):
			CurLine(stream->ReadLine(consumeCRLF)), Terminator(terminator),
			MyStream(stream), UseTerminator(useTerminator), ConsumeCRLF(consumeCRLF)
		{
			mIsEmpty = UseTerminator? (CurLine==Terminator): false;
			if(CurLine.Empty() && MyStream->EndOfStream()) mIsEmpty=true;
		}

		ByLineResult(const ByLineResult&) = delete;
		ByLineResult& operator=(const ByLineResult&) = delete;

		ByLineResult(ByLineResult&& rhs):
			CurLine(Meta::Move(rhs.CurLine)),
			Terminator(Meta::Move(rhs.Terminator)),
			MyStream(rhs.MyStream),
			UseTerminator(rhs.UseTerminator),
			ConsumeCRLF(rhs.ConsumeCRLF) {}

		ByLineResult& operator=(ByLineResult&& rhs)
		{
			CurLine = Meta::Move(rhs.CurLine);
			Terminator = Meta::Move(rhs.Terminator);
			MyStream = rhs.MyStream;
			UseTerminator = rhs.UseTerminator;
			ConsumeCRLF = rhs.ConsumeCRLF;
			return *this;
		}

		forceinline StringView First() const
		{
			INTRA_ASSERT(!Empty());
			return CurLine;
		}

		void PopFirst()
		{
			INTRA_ASSERT(!Empty());
			CurLine = MyStream->ReadLine(ConsumeCRLF);
			if(UseTerminator && CurLine==Terminator) mIsEmpty=true;
		}

		forceinline bool Empty() const {return mIsEmpty;}

		forceinline bool operator==(const ByLineResult& rhs) const
		{
			if(mIsEmpty && rhs.mIsEmpty) return true;
			INTRA_ASSERT(MyStream!=rhs.MyStream || CurLine==rhs.CurLine);
			return MyStream==rhs.MyStream;
		}

	private:
		bool mIsEmpty;
	};

	ByLineResult ByLine()
	{
		return ByLineResult(this, true, false);
	}

	ByLineResult ByLine(const String& terminator)
	{
		return ByLineResult(this, true, true, terminator);
	}
};

class IOutputStream
{
public:
	virtual void WriteData(const void* data, size_t bytes)=0;

	template<typename T> Meta::EnableIfPod<T> Write(const T& value) {WriteData(&value, sizeof(T));}

	template<typename T> Meta::EnableIfNotPod<T> Write(const T& value)
	{
		byte tempBuffer[4096];
		Data::BinarySerializer serializer = Data::BinarySerializer(MemoryOutput(tempBuffer));
		const size_t requiredBufferSize = serializer.SerializedSizeOf(value);
		size_t allocatedBufferSize = requiredBufferSize;

		if(requiredBufferSize>sizeof(tempBuffer))
			serializer.Output = MemoryOutput(ArrayRange<byte>(
				Memory::GlobalHeap.Allocate(allocatedBufferSize, INTRA_SOURCE_INFO),
				requiredBufferSize));

		serializer(value);
		WriteData(serializer.Output.Begin, serializer.Output.BytesWritten());

		if(requiredBufferSize>sizeof(tempBuffer))
			Memory::GlobalHeap.Free(tempBuffer, allocatedBufferSize);
	}

	//! Аналогично вызову Write<StringView>, но быстрее
	void WriteString(StringView s)
	{
		Write<uintLE>(uint(s.Length()));
		WriteData(s.Data(), s.Length());
	}

	virtual ~IOutputStream() {}

	forceinline void RawPrint(StringView s) {WriteData(s.Data(), s.Length());}
	virtual void Print(StringView s) {RawPrint(s);}

	template<typename Arg0> void Print(const Arg0& t)
	{
		Print(StringView(ToString(t)));
	}

	template<typename Arg0, typename Arg1, typename... Args>
	void Print(const Arg0& arg0, const Arg1& arg1, const Args&... args)
	{
		Print(arg0);
		Print(arg1, args...);
	}

	template<typename ...Args>
	void PrintLine(const Args&... args)
	{
		Print(args..., endl);
	}

	template<typename T> friend IOutputStream& operator<<(IOutputStream& stream, const T& n)
	{
		stream.Print(ToString(n));
		return stream;
	}

	virtual IOutputStream& operator<<(endl_t)
	{
		RawPrint(
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_WindowsPhone)
			"\r\n"
#else
			"\n"
#endif
		);
		return *this;
	}

	//! Сдвинуть текущую позицию записи на bytes байт
	//! Для корректной работы этого метода требуются правильно переопределённые GetPos и SetPos
	void Skip(ulong64 bytes)
	{
		auto curPos = GetPos();
		INTRA_ASSERT(curPos != Meta::NumericLimits<ulong64>::Max());
		SetPos(curPos+bytes);
	}

	bool IsSeekable() const {return GetPos()!=Meta::NumericLimits<ulong64>::Max();}
	virtual void SetPos(ulong64) {}
	virtual ulong64 GetSize() const {return Meta::NumericLimits<ulong64>::Max();} //По умолчанию поток будет считаться бесконечным
	virtual ulong64 GetPos() const {return Meta::NumericLimits<ulong64>::Max();} //По умолчанию у потока нет позиции. Возвращаемое значение (ulong64)-1 говорит о том, что поток не seekable
};

class ConsoleStream: public IInputStream, public IOutputStream
{
public:
	ConsoleStream(void* fout, void* fin);

	void WriteData(const void* dst, size_t n) override final; //Консоль трактует все данные как текст в формате UTF-8
	size_t ReadData(void* dst, size_t n) override final;
	void UnreadData(const void* data, size_t bytes) override final;
	void Skip(ulong64 bytes) {ReadNChars(size_t(bytes));}

	dchar GetChar();

private:
	virtual bool EndOfStream() const override {return false;}

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	byte unread_buf_chars;
	char unread_buf[5];
#endif

	void* myfout;
	void* myfin;

	ConsoleStream(const ConsoleStream&) = delete;
	ConsoleStream& operator=(const ConsoleStream&) = delete;
};

extern ConsoleStream Console, ConsoleError;

class MemoryInputStream: public IInputStream
{
public:
	MemoryInputStream(null_t=null): data(null), rest(null) {}

	MemoryInputStream(const void* memory, size_t length):
		data(reinterpret_cast<const byte*>(memory)),
		rest(reinterpret_cast<const byte*>(memory), length) {}

	MemoryInputStream(ArrayRange<const byte> memory):
		data(memory.Begin), rest(memory) {}

	MemoryInputStream(const MemoryInputStream& rhs) = default;
	MemoryInputStream& operator=(const MemoryInputStream& rhs) = default;

	size_t ReadData(void* dst, size_t bytes) override final
	{
		const auto bytesToRead = Math::Min(bytes, rest.Length());
		INTRA_ASSERT(bytesToRead<=bytes);
		C::memcpy(dst, rest.Begin, bytesToRead);
		rest.Begin+=bytesToRead;
		return bytesToRead;
	}

	void UnreadData(const void* src, size_t bytes) override final
	{
		(void)src;
		INTRA_ASSERT(C::memcmp(rest.Begin-bytes, src, bytes)==0);
		rest.Begin-=bytes;
		INTRA_ASSERT(rest.Begin>=data);
	}
	bool EndOfStream() const override {return rest.Empty();}

	void SetPos(ulong64 bytes) override final
	{
		rest.Begin = data+bytes;
		if(rest.Begin>rest.End) rest.Begin=rest.End;
	}

	ulong64 GetSize() const override final {return ulong64(rest.End-data);}
	ulong64 GetPos() const override final {return ulong64(rest.Begin-data);}

	const byte* data;
	ArrayRange<const byte> rest;
};

}}
