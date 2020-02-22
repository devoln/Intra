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

	//! ¬озвращает байтовый поток событий следующего трека.
	//! ѕеред вызовом любого метода этого класса необходимо, чтобы поток,
	//! возвращЄнный unsafe-методами был прочитан полностью или уничтожен.
	RTake<InputStream&> NextTrackByteStreamUnsafe();

	//! ¬озвращает поток событий следующего трека.
	//! ѕеред вызовом любого метода этого класса необходимо, чтобы поток,
	//! возвращЄнный unsafe-методами был прочитан полностью или уничтожен.
	MidiRawEventStream NextTrackRawEventStreamUnsafe();

	//! ¬озвращает парсер событий следующего трека.
	//! ѕеред вызовом любого метода этого класса необходимо, чтобы поток,
	//! возвращЄнный unsafe-методами был прочитан полностью или уничтожен.
	MidiTrackParser NextTrackParserUnsafe();


	//! ¬озвращает байтовый поток событий следующего трека.
	InputStream NextTrackByteStream();

	//! ¬озвращает поток событий следующего трека.
	MidiRawEventStream NextTrackRawEventStream();

	//! ¬озвращает парсер событий следующего трека.
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
