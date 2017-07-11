#include "MidiFileParser.h"

#include "Cpp/Endianess.h"

#include "Range/Stream/RawRead.h"

#include "Container/Utility/OwningArrayRange.h"

namespace Intra { namespace Audio { namespace Midi {

Range::RTake<InputStream&> MidiFileParser::NextTrackByteStreamUnsafe()
{
	if(mTrack < mHeaderTracks || mMidiStream.Empty())
		return Range::RTake<InputStream&>(mMidiStream, 0);

	char chunkType[4];
	mMidiStream.RawReadTo(chunkType, 4);
	uint size = mMidiStream.RawRead<uintBE>();
	if(StringView::FromBuffer(chunkType) != "MTrk")
	{
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
{return OwningArrayRange<char>(NextTrackByteStreamUnsafe());}

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
	InputStream stream, IDevice* device, ErrorStatus& status)
{
	MidiFileParser parser(Cpp::Move(stream), status);
	TrackCombiner result(parser.HeaderTimeFormat(), device);
	while(parser.TracksLeft() > 0) result.AddTrack(parser.NextTrackParser());
	return result;
}

}}}
