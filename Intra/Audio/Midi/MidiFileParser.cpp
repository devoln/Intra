#include "MidiFileParser.h"

#include "Cpp/Endianess.h"
#include "Cpp/Warnings.h"

#include "Range/Stream/RawRead.h"

#include "Container/Utility/OwningArrayRange.h"
#include "Container/Associative/HashMap.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Midi {

Range::RTake<InputStream&> MidiFileParser::NextTrackByteStreamUnsafe()
{
	if(mTrack == mHeaderTracks || mMidiStream.Empty())
	{
		mHeaderTracks = mTrack;
		return Range::RTake<InputStream&>(mMidiStream, 0);
	}

	char chunkType[4];
	mMidiStream.RawReadTo(chunkType, 4);
	uint size = mMidiStream.RawRead<uintBE>();
	if(StringView::FromBuffer(chunkType) != "MTrk")
	{
		mHeaderTracks = mTrack;
		mMidiStream.PopFirstN(size);
		mMidiStream = null;
		if(mErrStatus) mErrStatus->Error("Unexpected MIDI file block: expected MTrk.", INTRA_SOURCE_INFO);
		return Range::RTake<InputStream&>(mMidiStream, 0);
	}
	mTrack++;
	return Range::RTake<InputStream&>(mMidiStream, size);
}

RawEventStream MidiFileParser::NextTrackRawEventStreamUnsafe()
{return RawEventStream(NextTrackByteStreamUnsafe());}

TrackParser MidiFileParser::NextTrackParserUnsafe()
{return TrackParser(NextTrackRawEventStreamUnsafe());}

InputStream MidiFileParser::NextTrackByteStream()
{
	auto range = NextTrackByteStreamUnsafe();
	Array<char> arr;
	arr.SetCountUninitialized(range.LengthLimit());
	Range::ReadTo(range, arr);
	return OwningArrayRange<char>(Cpp::Move(arr));
}

RawEventStream MidiFileParser::NextTrackRawEventStream()
{return RawEventStream(NextTrackByteStream());}

TrackParser MidiFileParser::NextTrackParser()
{return TrackParser(NextTrackRawEventStream());}

MidiFileParser::MidiFileParser(InputStream stream, ErrorStatus& errStatus):
	mMidiStream(Cpp::Move(stream)), mErrStatus(&errStatus)
{
	if(!StartsAdvanceWith(mMidiStream, "MThd"))
	{
		mMidiStream = null;
		if(mErrStatus) mErrStatus->Error("Invalid MIDI file header!", INTRA_SOURCE_INFO);
		return;
	}
	const uint chunkSize = mMidiStream.RawRead<uintBE>();
	if(chunkSize != 6)
	{
		mMidiStream = null;
		if(mErrStatus) mErrStatus->Error(String::Concat("Invalid MIDI file header chunk size: ", chunkSize), INTRA_SOURCE_INFO);
		return;
	}
	mHeaderType = mMidiStream.RawRead<shortBE>();
	mHeaderTracks = mMidiStream.RawRead<shortBE>();
	mHeaderTimeFormat = mMidiStream.RawRead<shortBE>();

	if(mHeaderType > 2)
	{
		mMidiStream = null;
		if(mErrStatus) mErrStatus->Error(String::Concat("Invalid MIDI file header type: ", mHeaderType), INTRA_SOURCE_INFO);
		return;
	}
}

TrackCombiner MidiFileParser::CreateSingleOrderedMessageStream(
	InputStream stream, ErrorStatus& status)
{
	MidiFileParser parser(Cpp::Move(stream), status);
	TrackCombiner result(parser.HeaderTimeFormat());
	while(parser.TracksLeft() > 0) result.AddTrack(parser.NextTrackParser());
	return result;
}


MidiFileInfo::MidiFileInfo(InputStream stream, ErrorStatus& status)
{
	MidiFileParser parser(Cpp::Move(stream), status);
	Format = byte(parser.Type());

	TrackCombiner combiner(parser.HeaderTimeFormat());
	while(parser.TracksLeft() > 0) combiner.AddTrack(parser.NextTrackParser());

	TrackCount = ushort(parser.TrackCount());

	struct CountingDevice: IDevice
	{
		size_t NoteCount = 0;
		double Time = 0;
		size_t MaxSimultaneousNotes = 0;
		HashMap<ushort, bool> NoteMap;
		bool ChannelIsUsed[16]{false};
		StaticBitset<128>* UsedInstrumentsFlags;
		StaticBitset<128>* UsedDrumInstrumentsFlags;

		void OnNoteOn(const NoteOn& noteOn) final
		{
			ChannelIsUsed[noteOn.Channel] = true;
			const bool wasNote = NoteMap.Get(noteOn.Id(), false);
			if(wasNote && Math::Abs(Time - noteOn.Time) < 0.0001)
				return;
			NoteMap[noteOn.Id()] = true;
			if(noteOn.Channel == 9) UsedDrumInstrumentsFlags->Set(noteOn.NoteOctaveOrDrumId);
			else UsedInstrumentsFlags->Set(noteOn.Program);
			Time = noteOn.Time;
			if(MaxSimultaneousNotes < NoteMap.Count())
				MaxSimultaneousNotes = NoteMap.Count();
			NoteCount++;
		}

		void OnNoteOff(const NoteOff& noteOff) final
		{
			Time = noteOff.Time;
			NoteMap.Remove(noteOff.Id());
		}

		void OnAllNotesOff(byte channel) final
		{
			Array<HashMap<ushort, bool>::const_iterator> notesToRemove;
			for(auto it = NoteMap.begin(); it != NoteMap.end(); ++it)
			{
				if((it->Key >> 8) != channel) continue;
				notesToRemove.AddLast(it);
			}
			for(auto it: notesToRemove) NoteMap.Remove(it);
		}
	} countingDevice;
	countingDevice.UsedInstrumentsFlags = &UsedInstrumentsFlags;
	countingDevice.UsedDrumInstrumentsFlags = &UsedDrumInstrumentsFlags;

	combiner.ProcessAllEvents(countingDevice);

	NoteCount = countingDevice.NoteCount;
	Duration = countingDevice.Time;
	MaxSimultaneousNotes = countingDevice.MaxSimultaneousNotes;

	ChannelsUsed = 0;
	for(bool used: countingDevice.ChannelIsUsed) if(used) ChannelsUsed++;
}


}}}

INTRA_WARNING_POP
