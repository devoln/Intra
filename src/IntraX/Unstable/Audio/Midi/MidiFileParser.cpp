#include "MidiFileParser.h"

#include "IntraX/Utils/Endianess.h"
#include "Intra/Range/Stream/RawRead.h"
#include "IntraX/Container/Utility/OwningArrayRange.h"
#include "IntraX/Container/Associative/HashMap.h"

namespace Intra { INTRA_BEGIN
RTake<InputStream&> MidiFileParser::NextTrackByteStreamUnsafe()
{
	if(mTrack == mHeaderTracks || mMidiStream.Empty())
	{
		mHeaderTracks = mTrack;
		return RTake<InputStream&>(mMidiStream, 0);
	}

	char chunkType[4];
	mMidiStream.RawReadTo(chunkType, 4);
	uint32 size = mMidiStream.RawRead<uint32BE>();
	if(StringView::FromBuffer(chunkType) != "MTrk")
	{
		mHeaderTracks = mTrack;
		mMidiStream.PopFirstCount(index_t(size));
		mMidiStream = nullptr;
		mErr.Error(ErrorCode::InvalidStream, "Unexpected MIDI file block: expected MTrk.", INTRA_SOURCE_INFO);
		return RTake<InputStream&>(mMidiStream, 0);
	}
	mTrack++;
	return RTake<InputStream&>(mMidiStream, index_t(size));
}

MidiRawEventStream MidiFileParser::NextTrackRawEventStreamUnsafe()
{return MidiRawEventStream(NextTrackByteStreamUnsafe());}

MidiTrackParser MidiFileParser::NextTrackParserUnsafe()
{return MidiTrackParser(NextTrackRawEventStreamUnsafe());}

InputStream MidiFileParser::NextTrackByteStream()
{
	auto range = NextTrackByteStreamUnsafe();
	Array<char> arr;
	arr.SetCountUninitialized(range.LengthLimit());
	ReadTo(range, arr);
	return OwningArrayRange<char>(Move(arr));
}

MidiRawEventStream MidiFileParser::NextTrackRawEventStream()
{return MidiRawEventStream(NextTrackByteStream());}

MidiTrackParser MidiFileParser::NextTrackParser()
{return MidiTrackParser(NextTrackRawEventStream());}

MidiFileParser::MidiFileParser(InputStream stream, ErrorReporter err):
	mMidiStream(Move(stream)), mErr(err)
{
	if(!StartsAdvanceWith(mMidiStream, "MThd"))
	{
		mMidiStream = nullptr;
		mErr.Error(ErrorCode::InvalidStream, "Invalid MIDI file header!", INTRA_SOURCE_INFO);
		return;
	}
	const unsigned chunkSize = mMidiStream.RawRead<uint32BE>();
	if(chunkSize != 6)
	{
		mMidiStream = nullptr;
		mErr.Error(ErrorCode::InvalidStream, String::Concat("Invalid MIDI file header chunk size: ", chunkSize), INTRA_SOURCE_INFO);
		return;
	}
	mHeaderType = mMidiStream.RawRead<shortBE>();
	mHeaderTracks = mMidiStream.RawRead<shortBE>();
	mHeaderTimeFormat = mMidiStream.RawRead<shortBE>();

	if(mHeaderType > 2)
	{
		mMidiStream = nullptr;
		mErr.Error(ErrorCode::InvalidStream, String::Concat("Invalid MIDI file header type: ", mHeaderType), INTRA_SOURCE_INFO);
		return;
	}
}

MidiTrackCombiner MidiFileParser::CreateSingleOrderedMessageStream(
	InputStream stream, ErrorReporter err)
{
	MidiFileParser parser(Move(stream), err);
	MidiTrackCombiner result(parser.HeaderTimeFormat());
	while(parser.TracksLeft() > 0) result.AddTrack(parser.NextTrackParser());
	return result;
}


MidiFileInfo::MidiFileInfo(InputStream stream, ErrorReporter err)
{
	MidiFileParser parser(Move(stream), err);
	Format = byte(parser.Type());

	MidiTrackCombiner combiner(parser.HeaderTimeFormat());
	while(parser.TracksLeft() > 0) combiner.AddTrack(parser.NextTrackParser());

	TrackCount = uint16(parser.TrackCount());

	struct CountingDevice: IMidiDevice
	{
		size_t NoteCount = 0;
		MidiTime Time = 0;
		size_t MaxSimultaneousNotes = 0;
		HashMap<uint16, bool> NoteMap;
		bool ChannelIsUsed[16]{false};
		SBitset<128>* UsedInstrumentsFlags;
		SBitset<128>* UsedDrumInstrumentsFlags;

		void OnNoteOn(const MidiNoteOn& noteOn) final
		{
			ChannelIsUsed[noteOn.Channel] = true;
			const bool wasNote = NoteMap.Get(noteOn.Id(), false);
			if(wasNote && Time == noteOn.Time)
				return;
			NoteMap[noteOn.Id()] = true;
			if(noteOn.Channel == 9) UsedDrumInstrumentsFlags->Set(noteOn.NoteOctaveOrDrumId);
			else UsedInstrumentsFlags->Set(noteOn.Program);
			Time = noteOn.Time;
			if(MaxSimultaneousNotes < NoteMap.Count())
				MaxSimultaneousNotes = NoteMap.Count();
			NoteCount++;
		}

		void OnNoteOff(const MidiNoteOff& noteOff) final
		{
			Time = noteOff.Time;
			NoteMap.Remove(noteOff.Id());
		}

		void OnAllNotesOff(byte channel) final
		{
			Array<HashMap<uint16, bool>::const_iterator> notesToRemove;
			for(auto it = NoteMap.begin(); it != NoteMap.end(); ++it)
			{
				if((it->Field<0>() >> 8) != channel) continue;
				notesToRemove.AddLast(it);
			}
			for(auto it: notesToRemove) NoteMap.Remove(it);
		}
	} countingDevice;
	countingDevice.UsedInstrumentsFlags = &UsedInstrumentsFlags;
	countingDevice.UsedDrumInstrumentsFlags = &UsedDrumInstrumentsFlags;

	combiner.ProcessAllEvents(countingDevice);

	NoteCount = countingDevice.NoteCount;
	Duration = double(countingDevice.Time);
	MaxSimultaneousNotes = countingDevice.MaxSimultaneousNotes;

	ChannelsUsed = 0;
	for(bool used: countingDevice.ChannelIsUsed) if(used) ChannelsUsed++;
}
} INTRA_END
