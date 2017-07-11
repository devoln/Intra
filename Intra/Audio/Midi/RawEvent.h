#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"

#include "Utils/Debug.h"

#include "Range/Polymorphic/InputRange.h"

#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Midi {

struct RawEvent
{
	enum class Type: byte {
		NoteOff = 8, NoteOn, PolyphonicAftertouch, ControlModeChange,
		ProgramChange, ChannelAftertouch, PitchWheelRange, System
	};

	forceinline RawEvent(null_t=null): mData(7, '\0') {}

	RawEvent(uint delay, byte status, byte data0, byte data1, size_t metadataLength);

	RawEvent& operator=(null_t) {mData[4] = '\0'; return *this;}

	forceinline bool operator==(null_t) const noexcept {return mData.Get(4) == '\0';}
	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

	uint Delay() const noexcept
	{
		const byte* const data = reinterpret_cast<const byte*>(mData.Data());
		return uint(data[0]|(data[1] << 8)|(data[2] << 16)|(data[3] << 24));
	}

	forceinline byte Status() const {return byte(mData[4]);}
	forceinline Type MyType() const {return Type(Status() >> 4);}
	forceinline byte Channel() const {return byte(Status() & 15);}
	forceinline byte Data0() const {return byte(mData[5]);}
	forceinline byte Data1() const {return byte(mData[6]);}
	forceinline Span<byte> MetaData() noexcept {return mData.Drop(7).Reinterpret<byte>();}
	forceinline CSpan<byte> MetaData() const noexcept {return mData.Drop(7).Reinterpret<const byte>();}

private:
	//Благодаря Short String Optimization все события, не имеющие метаданных размера более 4/16 байт,
	//будут выделены на стеке и занимать sizeof(String) байт: 12/24 соответственно на 32 и 64-разрядных платформах
	String mData;
};

class RawEventStream
{
	RawEvent mCurrentEvent;
	byte mStatus = 0;
	InputStream mStream;

public:
	forceinline RawEventStream(null_t=null) {}

	forceinline RawEventStream(InputStream stream):
		mStream(Cpp::Move(stream)) {PopFirst();}

	RawEventStream(const RawEventStream&) = delete;
	RawEventStream& operator=(const RawEventStream&) = delete;
	RawEventStream(RawEventStream&&) = default;
	RawEventStream& operator=(RawEventStream&&) = default;

	void PopFirst();

	const RawEvent& First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mCurrentEvent;
	}

	forceinline bool Empty() const {return mCurrentEvent.Status() == 0;}
};

}}}

INTRA_WARNING_POP
