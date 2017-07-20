#pragma once

#include "Cpp/Warnings.h"

#include "Utils/ErrorStatus.h"

#include "Range/Decorators/Take.h"
#include "Range/Polymorphic/InputRange.h"

#include "Container/Utility/StaticBitset.h"

#include "RawEvent.h"
#include "TrackParser.h"
#include "TrackCombiner.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Midi {

class MidiFileParser
{
public:
	InputStream mMidiStream;

	short mHeaderType, mHeaderTracks, mHeaderTimeFormat;

	byte mTrack = 0;

	ErrorStatus* mErrStatus;

	explicit MidiFileParser(InputStream stream, ErrorStatus& errStatus);

	MidiFileParser(const MidiFileParser&) = delete;
	MidiFileParser& operator=(const MidiFileParser&) = delete;

	forceinline size_t TrackCount() const {return size_t(mHeaderTracks);}
	forceinline size_t TracksLeft() const {return size_t(mHeaderTracks - mTrack);}
	forceinline int Type() const {return mHeaderType;}
	forceinline short HeaderTimeFormat() const {return mHeaderTimeFormat;}

	//! ¬озвращает байтовый поток событий следующего трека.
	//! ѕеред вызовом любого метода этого класса необходимо, чтобы поток,
	//! возвращЄнный unsafe-методами был прочитан полностью или уничтожен.
	Range::RTake<InputStream&> NextTrackByteStreamUnsafe();

	//! ¬озвращает поток событий следующего трека.
	//! ѕеред вызовом любого метода этого класса необходимо, чтобы поток,
	//! возвращЄнный unsafe-методами был прочитан полностью или уничтожен.
	RawEventStream NextTrackRawEventStreamUnsafe();

	//! ¬озвращает парсер событий следующего трека.
	//! ѕеред вызовом любого метода этого класса необходимо, чтобы поток,
	//! возвращЄнный unsafe-методами был прочитан полностью или уничтожен.
	TrackParser NextTrackParserUnsafe();


	//! ¬озвращает байтовый поток событий следующего трека.
	InputStream NextTrackByteStream();

	//! ¬озвращает поток событий следующего трека.
	RawEventStream NextTrackRawEventStream();

	//! ¬озвращает парсер событий следующего трека.
	TrackParser NextTrackParser();

	static TrackCombiner CreateSingleOrderedMessageStream(InputStream stream, ErrorStatus& status);
};

struct MidiFileInfo
{
	explicit MidiFileInfo(InputStream stream, ErrorStatus& status);

	byte Format;
	ushort TrackCount;
	ushort ChannelsUsed;
	size_t NoteCount, MaxSimultaneousNotes;
	double Duration;
	float MaxVolume;
	StaticBitset<128> UsedInstrumentsFlags;
	StaticBitset<128> UsedDrumInstrumentsFlags;
};

}}}

INTRA_WARNING_POP
