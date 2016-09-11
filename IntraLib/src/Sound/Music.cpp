#include "Sound/Music.h"
#include "Sound/SoundBuilder.h"

namespace Intra {

using namespace Math;

const float MusicNote::BasicFrequencies[12] = {
	16.352f, 17.324f, 18.354f, 19.445f, 20.602f, 21.827f,
	23.125f, 24.500f, 25.957f, 27.500f, 29.135f, 30.868f
};


double MusicTrack::Duration() const
{
	double lastBeginOfNote=0, result=0;
	for(uint i=0; i<Notes.Count(); i++)
	{
		lastBeginOfNote += GetNoteTimeOffset(i);
		result = Max(result, lastBeginOfNote+Notes[i].Note.AbsDuration(Tempo));
	}
	return result;
}

MusicNote MusicTrack::operator[](uint index) const
{
	auto result = Notes[index].Note;
	if(result.IsPause() || ToneOffset==0) return result;
	result.Note = MusicNote::NoteType(result.Note+ToneOffset%12);
	if(result.Note>=12)
	{
		result.Note = MusicNote::NoteType(result.Note-12);
		result.Octave++;
	}
	result.Octave = byte(result.Octave + ToneOffset/12);
	return result;
}

SoundBuffer MusicTrack::GetSamples(uint sampleRate) const
{
	INTRA_ASSERT(Instrument!=null);
	const auto duration = Duration();
	SoundBuffer result((uint)(duration*sampleRate), sampleRate);
	if(result.Samples==null) return result;
	core::memset(&result.Samples[0], 0, result.Samples.Count()*sizeof(result.Samples[0]));
	uint samplePos=0;
	for(uint i=0; i<Notes.Count(); i++)
	{
		samplePos += uint(GetNoteTimeOffset(i)*sampleRate);
		if(Notes[i].Note.IsPause()) continue;

		Instrument->GetNoteSamples(result.Samples(samplePos, $), operator[](i), Tempo, Volume*Notes[i].Volume, sampleRate);
	}
	return result;
}



double Music::Duration() const
{
	double result=0;
	for(auto& track: Tracks)
		result = Max(result, track.Duration());
	return result;
}



SoundBuffer Music::GetSamples(uint sampleRate) const
{
	MusicSoundSampleSource src(*this, sampleRate);
	SoundBuffer result(src.SampleCount());
	src.GetInterleavedSamples(result.Samples);
	return result;
}





#ifndef INTRA_NO_MUSIC_LOADER


MusicSoundSampleSource::MusicSoundSampleSource(const Music& mydata, uint sampleRate):
	ASoundSampleSource(sampleRate, 1), data(mydata), currentPositions(mydata.Tracks.Count()), processedSamplesToFlush(0)
{
	maxVolume = -1000000;
	sample_count = uint(data.Duration()*this->sampleRate);
	for(uint i=0; i<data.Tracks.Count(); i++)
		currentPositions.AddLast(Position{0,0});
	current_sample_pos = 0;
}

size_t MusicSoundSampleSource::LoadNextNonNormalizedSamples(uint maxFloatsToGet)
{
	const uint floatsToRead = Math::Min(maxFloatsToGet, uint(sample_count*channelCount-current_sample_pos));

	for(uint i=0; i<data.Tracks.Count(); i++)
	{
		auto& track = data.Tracks[i];
		auto& trackPosition = currentPositions[i];
		while(trackPosition.noteId<track.Notes.Count())
		{
			const auto sampleOffset = uint(track.GetNoteTimeOffset(trackPosition.noteId)*sampleRate);
			if(trackPosition.samplePos+sampleOffset>=floatsToRead) break;
			trackPosition.samplePos += sampleOffset;
			const auto noteInfo = track.Notes[trackPosition.noteId];
			const auto note = track[trackPosition.noteId];
			const float volume = track.Volume*noteInfo.Volume;
			if(!note.IsPause() && volume>0.0001f)
			{
				INTRA_ASSERT((int)trackPosition.samplePos>=0);
				auto sampleCount = track.Instrument->GetNoteSampleCount(note, track.Tempo, sampleRate);
				if(buffer.Samples.Count()<trackPosition.samplePos+sampleCount)
					buffer.Samples.SetCount(sampleCount+trackPosition.samplePos);
				track.Instrument->GetNoteSamples(buffer.Samples(trackPosition.samplePos, $).Take(sampleCount), note, track.Tempo, volume, sampleRate, true);
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
		current_sample_pos=0;
		for(auto& pos: currentPositions) pos.noteId=0;
		//buffer.Clear();
	}

	return floatsToRead;
}

size_t MusicSoundSampleSource::LoadNextNormalizedSamples(uint maxFloatsToGet)
{
	size_t floatsRead = LoadNextNonNormalizedSamples(maxFloatsToGet);
	auto minmax = buffer.GetMinMax(0, Math::Min(floatsRead, buffer.Samples.Count()));
	maxVolume = Math::Max( maxVolume, Math::Abs(minmax.first) );
	maxVolume = Math::Max( maxVolume, Math::Abs(minmax.second) );
	buffer.SetMinMax(-1.0f, 1.0f, 0, floatsRead, {-maxVolume, maxVolume});
	return floatsRead;
}

void MusicSoundSampleSource::FlushProcessedSamples()
{
	buffer.ShiftSamples(-(intptr)processedSamplesToFlush);
	current_sample_pos += processedSamplesToFlush;
	processedSamplesToFlush = 0;
}

size_t MusicSoundSampleSource::GetInterleavedSamples(ArrayRange<short> outShorts)
{
	size_t floatsRead = LoadNextNonNormalizedSamples((uint)outShorts.Length());
	auto minmax = buffer.GetMinMax(0, Math::Min(floatsRead, buffer.Samples.Count()));
	maxVolume = Math::Max(maxVolume, Math::Abs(minmax.first));
	maxVolume = Math::Max(maxVolume, Math::Abs(minmax.second));
	buffer.SetMinMax(-32767.9f, 32766.9f, 0, floatsRead, {-maxVolume, maxVolume});
	buffer.CastToShorts(0, outShorts(0, floatsRead));
	processedSamplesToFlush = floatsRead;
	FlushProcessedSamples();
	return floatsRead/channelCount;
}

size_t MusicSoundSampleSource::GetInterleavedSamples(ArrayRange<float> outFloats)
{
	size_t floatsRead = LoadNextNormalizedSamples((uint)outFloats.Length());
	processedSamplesToFlush = floatsRead;
	Memory::CopyBits(outFloats, buffer.Samples(0, floatsRead).AsConstRange());
	FlushProcessedSamples();
	return floatsRead/channelCount;
}

size_t MusicSoundSampleSource::GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats)
{
	INTRA_ASSERT(channelCount==outFloats.Length());
	INTRA_ASSERT(channelCount==1); //TODO: убрать это ограничение
	return GetInterleavedSamples(outFloats[0]);
}

Array<const void*> MusicSoundSampleSource::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* outType, bool* outInterleaved, size_t* outSamplesRead)
{
	(void)maxSamplesToRead;
	if(outType!=null) *outType=ValueType::Void;
	if(outInterleaved!=null) *outInterleaved=false;
	if(outSamplesRead!=null) *outSamplesRead=0;
	return null; //На предпоследнем шаге сэмплы имеют тип float, но не нормированы
}

#endif

}
