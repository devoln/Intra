#pragma once

#include "Intra/Assert.h"
#include "Intra/Range/Polymorphic/InputRange.h"
#include "IntraX/Container/Sequential/String.h"

namespace Intra { INTRA_BEGIN
struct MidiRawEvent
{
	enum class Type: byte {
		NoteOff = 8, NoteOn, PolyphonicAftertouch, ControlModeChange,
		ProgramChange, ChannelAftertouch, PitchWheelRange, System
	};

	MidiRawEvent(decltype(nullptr)=nullptr): mData(7, '\0') {}

	MidiRawEvent(unsigned delay, byte status, byte data0, byte data1, size_t metadataLength);

	MidiRawEvent& operator=(decltype(nullptr)) {mData[4] = '\0'; return *this;}

	[[nodiscard]] bool operator==(decltype(nullptr)) const noexcept {return mData.View().Get(4) == '\0';}
	[[nodiscard]] bool operator!=(decltype(nullptr)) const noexcept {return !operator==(nullptr);}

	[[nodiscard]] unsigned Delay() const noexcept
	{
		const byte* const data = reinterpret_cast<const byte*>(mData.Data());
		return unsigned(data[0]|(data[1] << 8)|(data[2] << 16)|(data[3] << 24));
	}

	[[nodiscard]] byte Status() const {return byte(mData[4]);}
	[[nodiscard]] Type MyType() const {return Type(Status() >> 4);}
	[[nodiscard]] byte Channel() const {return byte(Status() & 15);}
	[[nodiscard]] byte Data0() const {return byte(mData[5]);}
	[[nodiscard]] byte Data1() const {return byte(mData[6]);}
	[[nodiscard]] Span<byte> MetaData() noexcept {return mData.View().Drop(7).ReinterpretUnsafe<byte>();}
	[[nodiscard]] Span<const byte> MetaData() const noexcept {return mData.View().Drop(7).ReinterpretUnsafe<const byte>();}

private:
	//��������� Short String Optimization ��� �������, �� ������� ���������� ������� ����� 4/16 ����,
	//����� �������� �� ����� � �������� sizeof(String) ����: 12/24 �������������� �� 32 � 64-��������� ����������
	String mData;
};

class MidiRawEventStream
{
	MidiRawEvent mCurrentEvent;
	byte mStatus = 0;
	InputStream mStream;
public:
	MidiRawEventStream(decltype(nullptr)=nullptr) {}

	MidiRawEventStream(InputStream stream):
		mStream(Move(stream)) {PopFirst();}

	MidiRawEventStream(const MidiRawEventStream&) = delete;
	MidiRawEventStream& operator=(const MidiRawEventStream&) = delete;
	MidiRawEventStream(MidiRawEventStream&&) = default;
	MidiRawEventStream& operator=(MidiRawEventStream&&) = default;

	void PopFirst();

	[[nodiscard]] const MidiRawEvent& First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mCurrentEvent;
	}

	[[nodiscard]] bool Empty() const {return mStream.Empty();}
};
} INTRA_END
