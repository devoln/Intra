#pragma once

#include "Extra/System/Error.h"

#include "Intra/Range/Take.h"
#include "Intra/Range/Polymorphic/InputRange.h"

#include "Intra/Container/SBitset.h"

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

	int TrackCount() const {return mHeaderTracks;}
	int TracksLeft() const {return mHeaderTracks - mTrack;}
	int Type() const {return mHeaderType;}
	short HeaderTimeFormat() const {return mHeaderTimeFormat;}

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
	uint16 TrackCount;
	uint16 ChannelsUsed;
	size_t NoteCount, MaxSimultaneousNotes;
	double Duration;
	SBitset<128> UsedInstrumentsFlags;
	SBitset<128> UsedDrumInstrumentsFlags;
};
INTRA_END
