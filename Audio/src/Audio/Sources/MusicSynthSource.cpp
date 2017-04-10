#include "Audio/Sources/MusicSynthSource.h"
#include "Math/Math.h"
#include "Platform/CppWarnings.h"
#include "Algo/Mutation/Fill.h"

namespace Intra { namespace Audio { namespace Sources {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifndef INTRA_NO_MUSIC_LOADER


MusicSynthSource::MusicSynthSource(const Music& data, uint sampleRate):
	ASoundSource(sampleRate, 1),
	mData(data), mBuffer(), mCurrentSamplePos(0),
	mSampleCount(size_t(mData.Duration()*sampleRate)),
	mCurrentPositions(data.Tracks.Count()),
	mMaxVolume(-1000000), mProcessedSamplesToFlush(0)
{
	for(uint i=0; i<mData.Tracks.Count(); i++)
		mCurrentPositions.AddLast(Position{0,0});

	for(const MusicTrack& track: data.Tracks)
	{
		if(track.Instrument==null) continue;
		track.Instrument->PrepareToPlay(track, sampleRate);
	}
}

size_t MusicSynthSource::LoadNextNonNormalizedSamples(uint maxFloatsToGet)
{
	const uint floatsToRead = Math::Min(maxFloatsToGet, uint(mSampleCount*mChannelCount-mCurrentSamplePos));

	for(uint i=0; i<mData.Tracks.Count(); i++)
	{
		auto& track = mData.Tracks[i];
		auto& trackPosition = mCurrentPositions[i];
		while(trackPosition.noteId<track.Notes.Count())
		{
			const auto sampleOffset = uint(track.GetNoteTimeOffset(trackPosition.noteId)*mSampleRate);
			if(trackPosition.samplePos+sampleOffset>=floatsToRead) break;
			trackPosition.samplePos += sampleOffset;
			const MusicTrack::NoteEntry noteInfo = track.Notes[trackPosition.noteId];
			const MusicNote note = track[trackPosition.noteId];
			const float volume = track.Volume*noteInfo.Volume;

			size_t sampleCount = floatsToRead;
			if(!note.IsPause()) sampleCount = track.Instrument->GetNoteSampleCount(note, track.Tempo, mSampleRate);
			INTRA_DEBUG_ASSERT(int(trackPosition.samplePos)>=0);
			if(mBuffer.Samples.Count()<trackPosition.samplePos+sampleCount)
				mBuffer.Samples.SetCount(trackPosition.samplePos+sampleCount);
			
			if(!note.IsPause() && volume>0.0001f)
			{
				auto dstRange = Range::Drop(mBuffer.Samples, trackPosition.samplePos).Take(sampleCount);
				track.Instrument->GetNoteSamples(dstRange, note, track.Tempo, volume, mSampleRate, true);
			}
			trackPosition.noteId++;
		}
		if(trackPosition.noteId==track.Notes.Count())
		{
			trackPosition.samplePos=0;
			continue;
		}
		trackPosition.samplePos -= maxFloatsToGet;
	}

	if(floatsToRead<maxFloatsToGet)
	{
		mCurrentSamplePos = 0;
		for(Position& pos: mCurrentPositions) pos.noteId=0;
		//buffer.Clear();
	}

	return floatsToRead;
}

size_t MusicSynthSource::LoadNextNormalizedSamples(uint maxFloatsToGet)
{
	size_t floatsRead = LoadNextNonNormalizedSamples(maxFloatsToGet);
	auto minmax = mBuffer.GetMinMax(0, Math::Min(floatsRead, mBuffer.Samples.Count()));
	mMaxVolume = Math::Max( mMaxVolume, Math::Abs(minmax.first) );
	mMaxVolume = Math::Max( mMaxVolume, Math::Abs(minmax.second) );
	if(mMaxVolume>=0.0025f) mBuffer.SetMinMax(-1.0f, 1.0f, 0, floatsRead, {-mMaxVolume, mMaxVolume});
	return floatsRead;
}

void MusicSynthSource::FlushProcessedSamples()
{
	mBuffer.ShiftSamples(-intptr(mProcessedSamplesToFlush));
	mCurrentSamplePos += mProcessedSamplesToFlush;
	mProcessedSamplesToFlush = 0;
}

size_t MusicSynthSource::GetInterleavedSamples(Span<short> outShorts)
{
	size_t floatsRead = LoadNextNonNormalizedSamples(uint(outShorts.Length()));
	auto minmax = mBuffer.GetMinMax(0, floatsRead);
	mMaxVolume = Math::Max(mMaxVolume, Math::Abs(minmax.first));
	mMaxVolume = Math::Max(mMaxVolume, Math::Abs(minmax.second));
	if(mMaxVolume>=0.0025f) mBuffer.SetMinMax(-32767.9f, 32766.9f, 0, floatsRead, {-mMaxVolume, mMaxVolume});
	mBuffer.CastToShorts(0, outShorts.Take(floatsRead));
	mProcessedSamplesToFlush = floatsRead;
	FlushProcessedSamples();
	return floatsRead/mChannelCount;
}

size_t MusicSynthSource::GetInterleavedSamples(Span<float> outFloats)
{
	const size_t floatsRead = LoadNextNormalizedSamples(uint(outFloats.Length()));
	Algo::CopyToAdvance(Range::Take(mBuffer.Samples, floatsRead), outFloats);
	Algo::FillZeros(outFloats);
	mProcessedSamplesToFlush = floatsRead;
	FlushProcessedSamples();
	return floatsRead/mChannelCount;
}

size_t MusicSynthSource::GetUninterleavedSamples(CSpan<Span<float>> outFloats)
{
	INTRA_DEBUG_ASSERT(mChannelCount==outFloats.Length());
	INTRA_DEBUG_ASSERT(mChannelCount==1); //TODO: убрать это ограничение
	return GetInterleavedSamples(outFloats.First());
}

Array<const void*> MusicSynthSource::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* oType, bool* oInterleaved, size_t* oSamplesRead)
{
	(void)maxSamplesToRead;
	if(oType!=null) *oType=ValueType::Void;
	if(oInterleaved!=null) *oInterleaved=false;
	if(oSamplesRead!=null) *oSamplesRead=0;
	return null; //На предпоследнем шаге сэмплы имеют тип float, но не нормированы
}

#endif

INTRA_WARNING_POP

}}}
