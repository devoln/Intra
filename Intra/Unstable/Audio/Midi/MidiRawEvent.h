#pragma once

#include "Core/Assert.h"
#include "Core/Range/Polymorphic/InputRange.h"
#include "Container/Sequential/String.h"

INTRA_BEGIN
struct MidiRawEvent
{
	enum class Type: byte {
		NoteOff = 8, NoteOn, PolyphonicAftertouch, ControlModeChange,
		ProgramChange, ChannelAftertouch, PitchWheelRange, System
	};

	forceinline MidiRawEvent(null_t=null): mData(7, '\0') {}

	MidiRawEvent(uint delay, byte status, byte data0, byte data1, size_t metadataLength);

	MidiRawEvent& operator=(null_t) {mData[4] = '\0'; return *this;}

	INTRA_NODISCARD forceinline bool operator==(null_t) const noexcept {return mData.Get(4) == '\0';}
	INTRA_NODISCARD forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

	INTRA_NODISCARD uint Delay() const noexcept
	{
		const byte* const data = reinterpret_cast<const byte*>(mData.Data());
		return uint(data[0]|(data[1] << 8)|(data[2] << 16)|(data[3] << 24));
	}

	INTRA_NODISCARD forceinline byte Status() const {return byte(mData[4]);}
	INTRA_NODISCARD forceinline Type MyType() const {return Type(Status() >> 4);}
	INTRA_NODISCARD forceinline byte Channel() const {return byte(Status() & 15);}
	INTRA_NODISCARD forceinline byte Data0() const {return byte(mData[5]);}
	INTRA_NODISCARD forceinline byte Data1() const {return byte(mData[6]);}
	INTRA_NODISCARD forceinline Span<byte> MetaData() noexcept {return mData.Drop(7).Reinterpret<byte>();}
	INTRA_NODISCARD forceinline CSpan<byte> MetaData() const noexcept {return mData.Drop(7).Reinterpret<const byte>();}

private:
	//Благодаря Short String Optimization все события, не имеющие метаданных размера более 4/16 байт,
	//будут выделены на стеке и занимать sizeof(String) байт: 12/24 соответственно на 32 и 64-разрядных платформах
	String mData;
};

class MidiRawEventStream
{
	MidiRawEvent mCurrentEvent;
	byte mStatus = 0;
	InputStream mStream;
public:
	forceinline MidiRawEventStream(null_t=null) {}

	forceinline MidiRawEventStream(InputStream stream):
		mStream(Move(stream)) {PopFirst();}

	MidiRawEventStream(const MidiRawEventStream&) = delete;
	MidiRawEventStream& operator=(const MidiRawEventStream&) = delete;
	MidiRawEventStream(MidiRawEventStream&&) = default;
	MidiRawEventStream& operator=(MidiRawEventStream&&) = default;

	void PopFirst();

	INTRA_NODISCARD const MidiRawEvent& First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mCurrentEvent;
	}

	INTRA_NODISCARD forceinline bool Empty() const {return mStream.Empty();}
};
INTRA_END
