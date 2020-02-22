#pragma once

#include "System/Error.h"

#include "Core/Range/Take.h"
#include "Core/Range/Polymorphic/InputRange.h"

#include "Utils/SBitset.h"

#include "MidiRawEvent.h"
#include "MidiTrackParser.h"
#include "MidiTrackCombiner.h"

INTRA_BEGIN
class MidiFileParser
{
public:
	InputStream mMidiStream;

	short mHeaderType, mHeaderTracks, mHeaderTimeFormat;

	byte mTrack = 0;

	ErrorReporter mErr;

	explicit MidiFileParser(InputStream stream, ErrorReporter err);

	MidiFileParser(const MidiFileParser&) = delete;
	MidiFileParser& operator=(const MidiFileParser&) = delete;

	forceinline size_t TrackCount() const {return size_t(mHeaderTracks);}
	forceinline size_t TracksLeft() const {return size_t(mHeaderTracks - mTrack);}
	forceinline int Type() const {return mHeaderType;}
	forceinline short HeaderTimeFormat() const {return mHeaderTimeFormat;}

	//! ���������� �������� ����� ������� ���������� �����.
	//! ����� ������� ������ ������ ����� ������ ����������, ����� �����,
	//! ������������ unsafe-�������� ��� �������� ��������� ��� ���������.
	RTake<InputStream&> NextTrackByteStreamUnsafe();

	//! ���������� ����� ������� ���������� �����.
	//! ����� ������� ������ ������ ����� ������ ����������, ����� �����,
	//! ������������ unsafe-�������� ��� �������� ��������� ��� ���������.
	MidiRawEventStream NextTrackRawEventStreamUnsafe();

	//! ���������� ������ ������� ���������� �����.
	//! ����� ������� ������ ������ ����� ������ ����������, ����� �����,
	//! ������������ unsafe-�������� ��� �������� ��������� ��� ���������.
	MidiTrackParser NextTrackParserUnsafe();


	//! ���������� �������� ����� ������� ���������� �����.
	InputStream NextTrackByteStream();

	//! ���������� ����� ������� ���������� �����.
	MidiRawEventStream NextTrackRawEventStream();

	//! ���������� ������ ������� ���������� �����.
	MidiTrackParser NextTrackParser();

	static MidiTrackCombiner CreateSingleOrderedMessageStream(InputStream stream, ErrorReporter err);
};

struct MidiFileInfo
{
	explicit MidiFileInfo(InputStream stream, ErrorReporter err);

	byte Format;
	ushort TrackCount;
	ushort ChannelsUsed;
	size_t NoteCount, MaxSimultaneousNotes;
	double Duration;
	SBitset<128> UsedInstrumentsFlags;
	SBitset<128> UsedDrumInstrumentsFlags;
};
INTRA_END
