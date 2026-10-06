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

static void addAllTracks(MidiFileParser& parser, TrackCombiner& combiner)
{
	while(parser.TracksLeft() > 0) combiner.AddTrack(parser.NextTrackParser());
}

static noinline void ReportParserError(InputStream& stream, ErrorStatus* status,
	StringView message, Utils::SourceInfo source)
{
	stream = null;
	if(status) status->Error(message, source);
}

MidiFileParser::MidiFileParser(InputStream stream, ErrorStatus& errStatus):
	mMidiStream(Cpp::Move(stream)), mErrStatus(&errStatus)
{
	if(!StartsAdvanceWith(mMidiStream, "MThd"))
	{
		ReportParserError(mMidiStream, mErrStatus, "Invalid MIDI file header!", INTRA_SOURCE_INFO);
		return;
	}
	const uint chunkSize = mMidiStream.RawRead<uintBE>();
	if(chunkSize != 6)
	{
		ReportParserError(mMidiStream, mErrStatus,
			String::Concat("Invalid MIDI file header chunk size: ", chunkSize), INTRA_SOURCE_INFO);
		return;
	}
	mHeaderType = mMidiStream.RawRead<shortBE>();
	mHeaderTracks = mMidiStream.RawRead<shortBE>();
	mHeaderTimeFormat = mMidiStream.RawRead<shortBE>();

	if(mHeaderType > 2)
	{
		ReportParserError(mMidiStream, mErrStatus,
			String::Concat("Invalid MIDI file header type: ", mHeaderType), INTRA_SOURCE_INFO);
		return;
	}
}

TrackCombiner MidiFileParser::CreateSingleOrderedMessageStream(
	InputStream stream, ErrorStatus& status)
{
	MidiFileParser parser(Cpp::Move(stream), status);
	TrackCombiner result(parser.HeaderTimeFormat());
	addAllTracks(parser, result);
	return result;
}


MidiFileSummary::MidiFileSummary(InputStream stream, ErrorStatus& status)
{
	MidiFileParser parser(Cpp::Move(stream), status);
	TrackCombiner combiner(parser.HeaderTimeFormat());
	addAllTracks(parser, combiner);

	TrackCount = ushort(parser.TrackCount());

	struct CountingDevice: IDevice
	{
		size_t NoteCount = 0;
		double Time = 0;
		byte ActiveNotes[16][128]{};
		bool ChannelIsUsed[16]{};

		void OnNoteOn(const NoteOn& noteOn) final
		{
			ChannelIsUsed[noteOn.Channel] = true;
			auto& state = ActiveNotes[noteOn.Channel][noteOn.NoteOctaveOrDrumId];
			if((state & 2) != 0 && Math::Abs(Time - noteOn.Time) < 0.0001)
				return;
			Time = noteOn.Time;
			NoteCount++;
			state = byte(1 | (noteOn.TotalVolume() != 0 ? 2 : 0));
		}

		void OnNoteOff(const NoteOff& noteOff) final
		{
			Time = noteOff.Time;
			auto& state = ActiveNotes[noteOff.Channel][noteOff.NoteOctaveOrDrumId];
			if((state & 1) == 0) return;
			state = 0;
		}

		void OnPitchBend(const PitchBend&) final {}

		void OnAllNotesOff(byte channel) final
		{
			for(byte note = 0; note < 128; ++note)
				ActiveNotes[channel][note] = 0;
		}
	} countingDevice;

	combiner.ProcessAllEvents(countingDevice);

	NoteCount = countingDevice.NoteCount;
	Duration = countingDevice.Time;
	ChannelsUsed = 0;
	for(bool used: countingDevice.ChannelIsUsed) if(used) ChannelsUsed++;
}


MidiFileInfo::MidiFileInfo(InputStream stream, ErrorStatus& status)
{
	MidiFileParser parser(Cpp::Move(stream), status);
	Format = byte(parser.Type());

	TrackCombiner combiner(parser.HeaderTimeFormat());
	addAllTracks(parser, combiner);

	TrackCount = ushort(parser.TrackCount());

	struct CountingDevice: IDevice
	{
		size_t NoteCount = 0;
		double Time = 0;
		float CurrentVolume = 0, MaxVolume = 0;
		size_t MaxSimultaneousNotes = 0;
		HashMap<ushort, float> NoteVolumeMap;
		bool ChannelIsUsed[16]{false};
		StaticBitset<128>* UsedInstrumentsFlags;
		StaticBitset<128>* UsedDrumInstrumentsFlags;
		StaticBitset<128>* UsedKeysPerInstrumentFlags;

		void OnNoteOn(const NoteOn& noteOn) final
		{
			ChannelIsUsed[noteOn.Channel] = true;
			auto& volume = NoteVolumeMap[noteOn.Id()];
			if(volume != 0 && Math::Abs(Time - noteOn.Time) < 0.0001)
				return;
			if(noteOn.Channel == 9) UsedDrumInstrumentsFlags->Set(noteOn.NoteOctaveOrDrumId);
			else
			{
				UsedInstrumentsFlags->Set(noteOn.Instrument);
				UsedKeysPerInstrumentFlags[noteOn.Instrument].Set(noteOn.NoteOctaveOrDrumId);
			}
			Time = noteOn.Time;
			if(MaxSimultaneousNotes < NoteVolumeMap.Count())
				MaxSimultaneousNotes = NoteVolumeMap.Count();
			NoteCount++;
			volume = noteOn.TotalVolume();
			CurrentVolume += volume;
			if(MaxVolume < CurrentVolume) MaxVolume = CurrentVolume;
		}

		void OnNoteOff(const NoteOff& noteOff) final
		{
			Time = noteOff.Time;
			auto found = NoteVolumeMap.Find(noteOff.Id());
			if(found.Empty())
				return;
			CurrentVolume -= found.First().Value;
			NoteVolumeMap.Remove(found);
		}

		void OnPitchBend(const PitchBend&) final {}

		void OnAllNotesOff(byte channel) final
		{
			Array<HashMap<ushort, float>::const_iterator> notesToRemove;
			for(auto it = NoteVolumeMap.begin(); it != NoteVolumeMap.end(); ++it)
			{
				if((it->Key >> 8) != channel) continue;
				CurrentVolume -= it->Value;
				notesToRemove.AddLast(it);
			}
			for(auto it: notesToRemove) NoteVolumeMap.Remove(it);
		}
	} countingDevice;
	countingDevice.UsedInstrumentsFlags = &UsedInstrumentsFlags;
	countingDevice.UsedDrumInstrumentsFlags = &UsedDrumInstrumentsFlags;
	countingDevice.UsedKeysPerInstrumentFlags = UsedKeysPerInstrument;

	combiner.ProcessAllEvents(countingDevice);

	NoteCount = countingDevice.NoteCount;
	Duration = countingDevice.Time;
	MaxSimultaneousNotes = countingDevice.MaxSimultaneousNotes;
	MaxVolume = countingDevice.MaxVolume;

	ChannelsUsed = 0;
	for(bool used: countingDevice.ChannelIsUsed) if(used) ChannelsUsed++;
}


}}}

INTRA_WARNING_POP
